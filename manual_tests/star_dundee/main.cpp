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
#include "SpaceWireBridges.hpp"
#include "SpaceWireZMQ.hpp"
#include "ZMQClient.hpp"
#include "ZMQServer.hpp"
#include "config/Config.hpp"
#include "config/yaml_io.hpp"
#include <SpaceWirePP/rmap.hpp>
#include <cassert>
#include <cstdint>
#include <queue>
#include <vector>
#include <zmq.hpp>

class ZMQRMAPClient : public ZMQClient<topic_policy::merge_all_topics>
{
    zmq::context_t m_ctx;
    zmq::socket_t m_publisher;
    zmq::socket_t m_requests;

public:
    ZMQRMAPClient(Config cfg)
            : ZMQClient { { topics::types::RMAP }, cfg, topic_policy::merge_all_topics {} }
    {
    }


    void rmap_read(uint32_t address, uint32_t len)
    {
        static int id = 0;
        spw_packet packet { spacewire::rmap::read_request_buffer_size(), 0, "STAR-Dundee" };
        spacewire::rmap::build_read_request(254, 2, 32, address, id++, len, packet.data.data());
        send_packet(packet);
    }

    void rmap_write(uint32_t address, const std::vector<unsigned char>& data)
    {
        static int id = 0;

        spw_packet packet { spacewire::rmap::write_request_buffer_size(std::size(data)), 0,
            "STAR-Dundee" };
        spacewire::rmap::build_write_request(
            254, 2, 32, address, id++, data.data(), std::size(data), packet.data.data());
        send_packet(packet);
    }
};

const auto YML_Config = std::string(R"(
STAR-Dundee:
)");


TEST_CASE("ZMQ Server", "[]")
{
    std::vector<spw_packet> loopback_packets;
    ZMQServer server { {} };
    auto _ = SpaceWireBridges::setup(
        config_yaml::load_config<Config>(YML_Config), &(server.received_packets));
    std::this_thread::sleep_for(5ms);
    ZMQRMAPClient client { server.configuration() };
    1000 * [&]() {
        static int offset = 0;
        std::vector<unsigned char> data(1024UL);
        std::generate(
            std::begin(data), std::end(data), [value = offset]() mutable { return value++; });
        client.rmap_write(0x40000000 + offset, data);
        offset += 1024;
        while (!client.has_packets())
        {
            std::this_thread::sleep_for(100us);
        }
        auto responses = client.get_packets();
        REQUIRE(std::size(responses) == 1);
        REQUIRE(spacewire::rmap::is_rmap(responses[0].data.data()));
        REQUIRE(spacewire::rmap::is_rmap_write_response(responses[0].data.data()));
    };
    std::this_thread::sleep_for(5ms);
    1000 * [&]() {
        static int offset = 0;
        client.rmap_read(0x40000000 + offset, 1024);
        offset += 1024;
        while (!client.has_packets())
        {
            std::this_thread::sleep_for(100us);
        }
        auto responses = client.get_packets();
        REQUIRE(std::size(responses) == 1);
        REQUIRE(spacewire::rmap::is_rmap(responses[0].data.data()));
        REQUIRE(spacewire::rmap::is_rmap_read_response(responses[0].data.data()));
    };
    std::this_thread::sleep_for(5ms);
    server.close();
}
