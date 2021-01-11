#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#include <catch2/catch_reporter_tap.hpp>
#include <catch2/catch_reporter_teamcity.hpp>
#else
#include <catch.hpp>
#include <catch_reporter_tap.hpp>
#include <catch_reporter_teamcity.hpp>
#endif
#include "MockBridge.hpp"
#include "SpaceWireBridges.hpp"
#include "SpaceWireZMQ.hpp"
#include "ZMQClient.hpp"
#include "ZMQServer.hpp"
#include "config/Config.hpp"
#include "config/yaml_io.hpp"
#include <SpaceWirePP/SpaceWire.hpp>
#include <SpaceWirePP/rmap.hpp>
#include <cassert>
#include <cstdint>
#include <vector>


const std::vector<spw_packet> packets = []() {
    std::vector<spw_packet> packets;
    std::generate_n(std::back_inserter(packets), 100, []() {
        bool loopback = rand() & 1;
        std::vector<unsigned char> data;
        data.push_back(loopback);
        std::generate_n(std::back_inserter(data), rand() % 32760 + 8, []() { return rand(); });
        spacewire::fields::protocol_identifier(data.data())
            = spacewire::protocol_id_t::SPW_PROTO_ID_CCSDS;
        return spw_packet { std::move(data), static_cast<size_t>(rand()), "Mock" };
    });
    return packets;
}();

spw_packet random_rmap_packet()
{
    spw_packet packet { spacewire::rmap::read_request_buffer_size(), 0, "Mock" };
    spacewire::rmap::build_read_request(
        1, rand(), rand(), rand(), rand(), rand() % 32768, packet.data.data());
    return packet;
}

spw_packet random_ccsds_packet()
{
    spw_packet packet { 32, 0, "Mock" };
    spacewire::fields::destination_logical_address(packet.data.data()) = 1;
    spacewire::fields::protocol_identifier(packet.data.data())
        = spacewire::protocol_id_t::SPW_PROTO_ID_CCSDS;
    return packet;
}

TEST_CASE("ZMQ Server", "[]")
{
    std::vector<spw_packet> loopback_packets;
    ZMQServer server { {} };
    auto _ = SpaceWireBridges::setup(
        config_yaml::load_config<Config>(YML_Config), &(server.received_packets));
    std::this_thread::sleep_for(5ms);
    GIVEN("An RMAP only client")
    {
        ZMQClient client { { topics::types::RMAP }, server.configuration(),
            topic_policy::merge_all_topics {} };
        WHEN("RMAP packets are published")
        {
            10 * [&]() { client.send_packet(random_rmap_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them")
            {
                auto packets = client.get_packets();
                REQUIRE(std::size(packets) == 10);
            }
        }
        WHEN("CCSDS packets are published")
        {
            10 * [&]() { client.send_packet(random_ccsds_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should ignore them")
            {
                auto packets = client.get_packets();
                REQUIRE(std::size(packets) == 0);
            }
        }
    }
    GIVEN("An RMAP+CCSDS client with per topic queue")
    {
        ZMQClient client { { topics::types::RMAP, topics::types::CCSDS }, server.configuration(),
            topic_policy::per_topic_queue {} };
        WHEN("RMAP packets are published")
        {
            10 * [&]() { client.send_packet(random_rmap_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them inside RMAP queue")
            {
                REQUIRE(std::size(client.get_packets(topics::types::RMAP)) == 10);
                REQUIRE(std::size(client.get_packets(topics::types::CCSDS)) == 0);
            }
        }
        WHEN("CCSDS packets are published")
        {
            10 * [&]() { client.send_packet(random_ccsds_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them inside CCSDS queue")
            {
                REQUIRE(std::size(client.get_packets(topics::types::RMAP)) == 0);
                REQUIRE(std::size(client.get_packets(topics::types::CCSDS)) == 10);
            }
        }
    }
    GIVEN("An RMAP+CCSDS client with all topics in one queue")
    {
        ZMQClient client { { topics::types::RMAP, topics::types::CCSDS }, server.configuration(),
            topic_policy::merge_all_topics {} };
        WHEN("RMAP packets are published")
        {
            10 * [&]() { client.send_packet(random_rmap_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them inside queue")
            {
                REQUIRE(std::size(client.get_packets()) == 10);
            }
        }
        WHEN("CCSDS packets are published")
        {
            10 * [&]() { client.send_packet(random_ccsds_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them inside queue")
            {
                REQUIRE(std::size(client.get_packets()) == 10);
            }
        }
        WHEN("Both RMAP and CCSDS packets are published")
        {
            10 * [&]() { client.send_packet(random_ccsds_packet()); };
            10 * [&]() { client.send_packet(random_rmap_packet()); };
            std::this_thread::sleep_for(50ms);
            THEN("Client should receive and store them inside queue")
            {
                REQUIRE(std::size(client.get_packets()) == 20);
            }
        }
    }
    server.close();
}
