#define CATCH_CONFIG_MAIN
#if __has_include(<catch2/catch.hpp>)
#include <catch2/catch.hpp>
#else
#include <catch.hpp>
#endif
#include "SpaceWireBridge.hpp"
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
        if (packet.packet[0] == redirect_value)
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

static auto t = SpaceWireBriges::register_ctor(
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

    void send_packet(bool loopback)
    {
        if (m_requests.connected())
        {
            zmq::mutable_buffer resp;
            spw_packet packet { { loopback, 0, 0, 0 }, 0, "Mock" };
            m_requests.send(to_message(packet), zmq::send_flags::none);
            m_requests.recv(resp);
        }
    }

    int consume_packets()
    {
        zmq::message_t message;
        int count = 0;
        while (m_publisher.recv(message, zmq::recv_flags::dontwait))
        {
            count++;
        }
        return count;
    }
};

const auto YML_Config = std::string(R"(
Mock:
  redirect_value: 1
)");


TEST_CASE("ZMQ Server", "[]")
{
    int loopback_packets = 0;
    ZMQServer server { {} };
    SpaceWireBriges::setup(
        config_yaml::load_config<Config>(YML_Config), &(server.received_packets));
    std::this_thread::sleep_for(5ms);
    ZMQMockClient client { server.configuration() };
    repeat_n(
        [&]() {
            bool loopback = rand() & 1;
            client.send_packet(loopback);
            loopback_packets += loopback;
        },
        100);
    std::this_thread::sleep_for(5ms);
    REQUIRE(std::size(sent_packets) == 100UL);
    REQUIRE(client.consume_packets() == loopback_packets);
    server.close();
    SpaceWireBriges::teardown();
}
