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
#include "ZMQServer.hpp"
#include "config/Config.hpp"
#include "config/yaml_io.hpp"
#include <SpaceWirePP/rmap.hpp>
#include <cassert>
#include <cstdint>
#include <queue>
#include <vector>
#include <zmq.hpp>

class ZMQMockClient
{
    zmq::context_t m_ctx;
    zmq::socket_t m_publisher;
    zmq::socket_t m_requests;

public:
    ZMQMockClient(Config cfg)
    {
        const auto address = cfg["address"].to<std::string>("127.0.0.1");
        const auto pub_port = cfg["pub_port"].to<int>(30000);
        const auto req_port = cfg["req_port"].to<int>(30001);

        m_ctx = zmq::context_t { 1 };
        m_publisher = zmq::socket_t { m_ctx, zmq::socket_type::sub };
        m_requests = zmq::socket_t { m_ctx, zmq::socket_type::req };

        m_publisher.connect(fmt::format("tcp://{}:{}", address, pub_port));
        m_publisher.set(zmq::sockopt::subscribe, "Packets");
        m_requests.connect(fmt::format("tcp://{}:{}", address, req_port));
    }

    void send_packet(const spw_packet& packet)
    {
        if (m_requests.connected())
        {
            zmq::mutable_buffer resp;
            m_requests.send(to_message(packet), zmq::send_flags::none);
            m_requests.recv(resp);
        }
    }

    void rmap_read(uint32_t address, uint32_t len)
    {
        static int id = 0;
        if (m_requests.connected())
        {
            spw_packet packet { spacewire::rmap::read_request_buffer_size(), 0, "STAR-Dundee" };
            spacewire::rmap::build_read_request(254, 2, 32, address, id++, len, packet.data.data());
            zmq::mutable_buffer resp;
            m_requests.send(to_message(packet), zmq::send_flags::none);
            m_requests.recv(resp);
        }
    }

    void rmap_write(uint32_t address, const std::vector<unsigned char>& data)
    {
        static int id = 0;
        if (m_requests.connected())
        {
            spw_packet packet { spacewire::rmap::write_request_buffer_size(std::size(data)), 0,
                "STAR-Dundee" };
            spacewire::rmap::build_write_request(
                254, 2, 32, address, id++, data.data(), std::size(data), packet.data.data());
            zmq::mutable_buffer resp;
            m_requests.send(to_message(packet), zmq::send_flags::none);
            m_requests.recv(resp);
        }
    }

    std::vector<spw_packet> consume_packets(int count = 100, bool wait = false)
    {
        zmq::message_t message;
        std::vector<spw_packet> packets;
        packets.reserve(100);
        while (count--
            && m_publisher.recv(message, wait ? zmq::recv_flags::none : zmq::recv_flags::dontwait))
        {
            packets.push_back(to_packet("Packets", message));
        }
        return packets;
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
    ZMQMockClient client { server.configuration() };
    1000 * [&]() {
        static int offset = 0;
        std::vector<unsigned char> data(1024UL);
        std::generate(
            std::begin(data), std::end(data), [value = offset]() mutable { return value++; });
        client.rmap_write(0x40000000 + offset, data);
        offset += 1024;
        auto responses = client.consume_packets(1, true);
        REQUIRE(std::size(responses) == 1);
        REQUIRE(spacewire::rmap::is_rmap(responses[0].data.data()));
        REQUIRE(spacewire::rmap::is_rmap_write_response(responses[0].data.data()));
    };
    std::this_thread::sleep_for(5ms);
    1000 * [&]() {
        static int offset = 0;
        client.rmap_read(0x40000000 + offset, 1024);
        std::this_thread::sleep_for(500us);
        offset += 1024;
        auto responses = client.consume_packets(1, true);
        REQUIRE(std::size(responses) == 1);
        REQUIRE(spacewire::rmap::is_rmap(responses[0].data.data()));
        REQUIRE(spacewire::rmap::is_rmap_read_response(responses[0].data.data()));
    };
    std::this_thread::sleep_for(5ms);
    server.close();
}
