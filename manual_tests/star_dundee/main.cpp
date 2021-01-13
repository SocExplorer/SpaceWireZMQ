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
        rmap_write(address, data.data(), std::size(data));
    }

    void rmap_write(uint32_t address, const unsigned char* data, std::size_t size)
    {
        static int id = 0;
        spw_packet packet { spacewire::rmap::write_request_buffer_size(size), 0, "STAR-Dundee" };
        spacewire::rmap::build_write_request(
            254, 2, 32, address, id++, data, size, packet.data.data());
        send_packet(packet);
    }
};

const auto YML_Config = std::string(R"(
STAR-Dundee:
)");


TEST_CASE("ZMQ Server", "[]")
{
    constexpr auto bucket_size = 1024UL;
    constexpr auto bucket_count = 1024UL;
    using namespace spacewire::rmap;
    std::vector<unsigned char> ref_data(bucket_count * bucket_size);
    std::generate(std::begin(ref_data), std::end(ref_data), []() mutable { return rand(); });
    std::vector<spw_packet> loopback_packets;
    ZMQServer server { {} };
    auto _ = SpaceWireBridges::setup(
        config_yaml::load_config<Config>(YML_Config), &(server.received_packets));
    std::this_thread::sleep_for(5ms);
    ZMQRMAPClient client { server.configuration() };
    bucket_count* [&]() {
        static int offset = 0;
        client.rmap_write(0x40000000 + offset, ref_data.data() + offset, bucket_size);
        offset += bucket_size;
        while (!client.has_packets())
        {
            std::this_thread::sleep_for(100us);
        }
        auto responses = client.get_packets();
        REQUIRE(std::size(responses) == 1);
        REQUIRE(is_rmap(responses[0].data.data()));
        REQUIRE(is_rmap_write_response(responses[0].data.data()));
    };
    std::this_thread::sleep_for(5ms);
    bucket_count* [&]() {
        static int offset = 0;
        client.rmap_read(0x40000000 + offset, bucket_size);
        while (!client.has_packets())
        {
            std::this_thread::sleep_for(100us);
        }
        auto responses = client.get_packets();
        REQUIRE(std::size(responses) == 1);
        const auto packet = responses[0].data.data();
        REQUIRE(is_rmap(packet));
        REQUIRE(is_rmap_read_response(packet));
        REQUIRE(header_crc_valid<rmap_read_response_tag>(packet));
        REQUIRE(data_crc_valid<rmap_read_response_tag>(packet));
        for (auto i = 0UL; i < bucket_size; i++)
        {
            REQUIRE(fields::data<rmap_read_response_tag>(packet)[i] == ref_data[i + offset]);
        }
        offset += bucket_size;
    };
    std::this_thread::sleep_for(5ms);
    server.close();
}
