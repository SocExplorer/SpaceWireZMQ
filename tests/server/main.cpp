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

TEST_CASE("ZMQ Server", "[]")
{
    std::vector<spw_packet> loopback_packets;
    ZMQServer server { {} };
    auto _ = SpaceWireBridges::setup(
        config_yaml::load_config<Config>(YML_Config), &(server.received_packets));
    std::this_thread::sleep_for(5ms);
    ZMQClient<topic_policy::per_topic_queue> client { { topics::types::CCSDS },
        server.configuration() };
    std::for_each(std::cbegin(packets), std::cend(packets), [&](const spw_packet& packet) {
        client.send_packet(packet);
        if (packet.data[0])
            loopback_packets.push_back(packet);
    });
    std::this_thread::sleep_for(5ms);
    REQUIRE(std::size(sent_packets) == 100UL);
    const auto received_loopback_packets = client.get_packets(topics::types::CCSDS);
    REQUIRE(std::size(received_loopback_packets) == std::size(loopback_packets));
    REQUIRE(received_loopback_packets == loopback_packets);
    server.close();
}
