/*------------------------------------------------------------------------------
--  This file is a part of the SocExplorer Software
--  Copyright (C) 2021, Plasma Physics Laboratory - CNRS
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
----------------------------------------------------------------------------*/
#pragma once
#include <algorithm>
#include <callable.hpp>
#include <cassert>
#include <cfg_api_brick_mk2.h>
#include <cfg_api_brick_mk3.h>
#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <star-api.h>
#include <string>
#include <strings/algorithms.hpp>
#include <thread>

namespace StarAPI
{

struct RawPacketBuffer
{
    unsigned char* data;
    std::size_t size;
    RawPacketBuffer(unsigned char* data, std::size_t size) : data { data }, size { size } { }
    virtual ~RawPacketBuffer() = default;
};

struct ManagedRawPacketBuffer : RawPacketBuffer
{

    ManagedRawPacketBuffer(unsigned char* data, std::size_t size) : RawPacketBuffer(data, size) { }

    ManagedRawPacketBuffer(ManagedRawPacketBuffer&& other) : RawPacketBuffer(other.data, other.size)
    {
        other.data = nullptr;
        other.size = 0UL;
    }

    ManagedRawPacketBuffer& operator=(ManagedRawPacketBuffer&& other)
    {
        if (data)
            STAR_destroyPacketData(data);
        data = other.data;
        size = other.size;
        other.data = nullptr;
        other.size = 0UL;
        return *this;
    }

    ~ManagedRawPacketBuffer()
    {
        if (data)
            STAR_destroyPacketData(data);
    }
};

inline std::optional<STAR_DEVICE_ID> find_device(const std::string& serial_number)
{
    std::optional<STAR_DEVICE_ID> maybe_dev { std::nullopt };
    unsigned int dev_count;
    auto dev_list = STAR_getDeviceList(&dev_count);
    std::for_each(dev_list, dev_list + dev_count, [&maybe_dev, &serial_number](STAR_DEVICE_ID dev) {
        auto SN = STAR_getDeviceSerialNumber(dev);
        if (serial_number == cpp_utils::strings::trim(std::string { SN }))
        {
            maybe_dev = dev;
        }
        STAR_destroyString(SN);
    });
    return maybe_dev;
}

class Device
{
    std::optional<STAR_DEVICE_ID> m_handle { std::nullopt };

public:
    inline auto handle() const { return m_handle.value(); }
    inline bool ready() const { return bool(m_handle); }

    bool set_speed(unsigned int speed, std::size_t port)
    {
        if ((2000000 <= speed) && (400000000 >= speed))
        {
            if (ready())
            {
                //@TODO!
                CFG_BRICK_MK3_setTransmitClock(
                    handle(), port, STAR_CFG_MK2_BASE_TRANSMIT_CLOCK { 2, 40 });
                return true;
            }
        }
        return false;
    }

    Device() = default;
    Device(const std::string& serial_number) { m_handle = find_device(serial_number); }
    ~Device() { }
};

enum class channel_direction_t
{
    in = STAR_CHANNEL_DIRECTION_IN,
    out = STAR_CHANNEL_DIRECTION_OUT,
    inout = STAR_CHANNEL_DIRECTION_INOUT
};

template <channel_direction_t dir>
inline constexpr bool is_input
    = (dir == channel_direction_t::in) or (dir == channel_direction_t::inout);

template <channel_direction_t dir>
inline constexpr bool is_output
    = (dir == channel_direction_t::out) or (dir == channel_direction_t::inout);

template <channel_direction_t direction_>
class Channel
{
    std::optional<STAR_CHANNEL_ID> m_handle { std::nullopt };
    std::list<ManagedRawPacketBuffer> m_received_packets;
    std::mutex m_packet_queue_mutex;
    STAR_TRANSFER_OPERATION* m_rx_operation;

    void register_rx_callback()
    {
        static_assert(is_input<direction>,
            "Registering reception callback on output only channel is an error!");
        m_rx_operation = STAR_createRxOperation(1, STAR_RECEIVE_PACKETS);
        if (m_rx_operation)
        {
            STAR_registerTransferCompletionListener(m_rx_operation,
                callableToPointer([this](STAR_TRANSFER_OPERATION* pOperation,
                                      STAR_TRANSFER_STATUS status, void* pContextInfo) {
                    if (status == STAR_TRANSFER_STATUS_COMPLETE)
                    {
                        std::lock_guard guard { m_packet_queue_mutex };
                        auto streamItem = STAR_getTransferItem(pOperation, 0);
                        unsigned int dataLength = 0;
                        unsigned char* data = STAR_getPacketData(
                            (STAR_SPACEWIRE_PACKET*)streamItem->item, &dataLength);
                        m_received_packets.push_front(ManagedRawPacketBuffer { data, dataLength });
                        STAR_destroyStreamItem(streamItem);
                    }
                    STAR_submitTransferOperation(*m_handle, m_rx_operation);
                }),
                nullptr);
            STAR_submitTransferOperation(*m_handle, m_rx_operation);
        }
    }

    inline void dispose_rx_callback()
    {
        static_assert(is_input<direction>,
            "Registering reception callback on output only channel is an error!");
        STAR_disposeTransferOperation(m_rx_operation);
    }

public:
    static constexpr auto direction = direction_;

    void open(const Device& dev, unsigned int dev_channel)
    {
        if (dev.ready())
        {
            m_handle = STAR_openChannelToLocalDevice(
                dev.handle(), static_cast<STAR_CHANNEL_DIRECTION>(direction), dev_channel, 1);
            if constexpr (is_input<direction>)
            {
                register_rx_callback();
            }
        }
    }

    inline bool ready() const { return bool(m_handle); }

    inline std::size_t available_packets_count()
    {
        std::lock_guard lock { m_packet_queue_mutex };
        return std::size(m_received_packets);
    }

    template <channel_direction_t d = direction_>
    inline typename std::enable_if_t<is_output<d>, void> send_packet(const RawPacketBuffer& buffer)
    {
        if (ready() && buffer.data)
            STAR_transmitPacket(*m_handle, buffer.data, buffer.size, STAR_EOP_TYPE_EOP, 0);
    }

    template <channel_direction_t d = direction_>
    inline typename std::enable_if_t<is_input<d>, ManagedRawPacketBuffer> receive_packet()
    {
        using namespace std::chrono_literals;
        while (1)
        {
            {
                std::lock_guard lock(m_packet_queue_mutex);
                if (std::size(m_received_packets))
                {
                    ManagedRawPacketBuffer p { std::move(m_received_packets.back()) };
                    m_received_packets.pop_back();
                    return p;
                }
            }
            // to avoid this, only call receive packet when packet queue isn't empty
            std::this_thread::sleep_for(100us);
        }
    }

    template <channel_direction_t d = direction_>
    typename std::enable_if_t<is_output<d>, Channel&> operator>>(const RawPacketBuffer& buffer)
    {
        send_packet(buffer);
        return *this;
    }

    template <channel_direction_t d = direction_>
    typename std::enable_if_t<is_input<d>, Channel&> operator<<(ManagedRawPacketBuffer& buffer)
    {
        buffer = receive_packet();
        return *this;
    }


    Channel() = default;
    Channel(const Device& dev, unsigned int dev_channel) { open(dev, dev_channel); }
    Channel(Channel&& other)
    {
        if (other.m_handle)
            throw std::runtime_error { "Can't move an open channel" };
    }
    ~Channel()
    {
        if (m_handle)
            STAR_closeChannel(*m_handle);
    }
};
}
