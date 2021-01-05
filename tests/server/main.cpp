#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#include <catch2/catch_reporter_teamcity.hpp>
#include <catch2/catch_reporter_tap.hpp>
#else
#include <catch.hpp>
#include <catch_reporter_teamcity.hpp>
#include <catch_reporter_tap.hpp>
#endif
#include "SpaceWireBridge.hpp"
#include "SpaceWireBridges.hpp"
#include "SpaceWireZMQ.hpp"
#include "ZMQServer.hpp"
#include "config/Config.hpp"
#include "config/yaml_io.hpp"
#include <cassert>
#include <cstdint>
#include <queue>
#include <vector>
#include <zmq.hpp>

static std::vector<spw_packet> sent_packets;
static std::queue<spw_packet> loopback_packets;

class MockBridge : public ISpaceWireBridge
{
    char redirect_value { 0 };

public:
    virtual bool send_packet(const spw_packet& packet) final
    {
        sent_packets.push_back(packet);
        if (packet.data[0] == redirect_value)
            loopback_packets.push(packet);
        return true;
    }
    virtual spw_packet receive_packet() final
    {
        auto packet = loopback_packets.front();
        loopback_packets.pop();
        return packet;
    }

    virtual bool packet_received() final { return std::size(loopback_packets); }
    virtual bool set_configuration(const Config& cfg) final
    {
        redirect_value = cfg["redirect_value"].to<int>(0);
        return true;
    }
    virtual Config configuration() const final { return {}; }
    MockBridge(const Config& cfg) { set_configuration(cfg); }
    virtual ~MockBridge() { }
};

static auto t = SpaceWireBridges::register_ctor(
    "Mock", [](const Config& cfg, packet_queue* publish_queue) {
        return std::make_unique<SpaceWireBridge>(std::make_unique<MockBridge>(cfg), publish_queue);
    });

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

    std::vector<spw_packet> consume_packets()
    {
        zmq::message_t message;
        std::vector<spw_packet> packets;
        packets.reserve(100);
        while (m_publisher.recv(message, zmq::recv_flags::dontwait))
        {
            packets.push_back(to_packet("Packets", message));
        }
        return packets;
    }
};

const auto YML_Config = std::string(R"(
Mock:
  redirect_value: 1
)");

const std::vector<spw_packet> packets = []() {
    std::vector<spw_packet> packets;
    std::generate_n(std::back_inserter(packets), 100, []() {
        bool loopback = rand() & 1;
        std::vector<char> data;
        data.push_back(loopback);
        std::generate_n(std::back_inserter(data), rand() % 32768, []() { return rand(); });
        return spw_packet { data, static_cast<size_t>(rand()), "Mock" };
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
    ZMQMockClient client { server.configuration() };
    std::for_each(std::cbegin(packets), std::cend(packets), [&](const spw_packet& packet) {
        client.send_packet(packet);
        if (packet.data[0])
            loopback_packets.push_back(packet);
    });
    std::this_thread::sleep_for(5ms);
    REQUIRE(std::size(sent_packets) == 100UL);
    const auto received_loopback_packets = client.consume_packets();
    REQUIRE(std::size(received_loopback_packets) == std::size(loopback_packets));
    REQUIRE(received_loopback_packets == loopback_packets);
    server.close();
}
