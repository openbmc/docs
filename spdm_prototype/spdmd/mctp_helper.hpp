/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION &
 * AFFILIATES. All rights reserved. SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

extern "C"
{
#include "library/spdm_return_status.h"
}

#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/mctp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <phosphor-logging/lg2.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

#define MCTP_TYPE_SPDM 5

using timeout_us_t = uint64_t; /// in units of 1 micro second
constexpr timeout_us_t timeoutUsInfinite =
    std::numeric_limits<timeout_us_t>::max();
constexpr timeout_us_t timeoutUsMaximum = timeoutUsInfinite - 1;

namespace spdm
{
// these are for use with the mctp-demux-daemon

constexpr size_t mctpMaxMessageSize = 4096;

/** @struct NonCopyable
 *  @brief Helper class for deleting copy ops
 *  @details We often don't needed/want these and clang-tidy complains about
 * them
 */
struct NonCopyable
{
    NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable& other) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;

    NonCopyable(NonCopyable&&) = delete;
    NonCopyable& operator=(NonCopyable&&) = delete;
};

/** @class IOClass
 *  @brief Abstract interface for writing/reading full transport+spdm packets
 * to/from some I/O medium, typically a socket, or buffers during unit-tests
 *  @details write will be called by ConnectionClass when it sends a packet,
 * read will be called by the application and the buffer provided to
 * ConnectionClass through handleRecv()
 */
// NOLINTNEXTLINE cppcoreguidelines-special-member-functions
class IOClass : NonCopyable
{
  public:
    virtual ~IOClass() = default;

    /** @brief function called by ConnectionClass when it has encoded a full
     * transport+spdm packet and wishes to send it
     *  @param[in] buf - buffer containing the data to be sent
     */
    virtual libspdm_return_t
        write(const std::vector<uint8_t>& buf,
              timeout_us_t timeout = timeoutUsInfinite) = 0;

    /** @brief function called by the application either synchronuously or after
     * receiving an event
     *  @param[out] buf - buffer into which the full packet data must be written
     */
    virtual libspdm_return_t read(std::vector<uint8_t>& buf,
                                  timeout_us_t timeout = timeoutUsInfinite) = 0;
};

/** @class MctpMessageTransport
 *  @brief Support class for transport through the mctp-demux-daemon
 *  @details This class handles encoding and decoding of MCTP transport data.
 */
class MctpMessageTransport : public NonCopyable
{
  public:
    virtual ~MctpMessageTransport() = default;

    /** @brief function called by ConnectionClass before encoding an spdm
     * message into a buffer
     *  @details it must write the size of the transport data into lay.Size,
     * besides that it can already write it's data into buf at lay.getOffset()
     *           afterwards the spdm message will be written at
     * buf[lay.getEndOffset()]
     *  @param[out] buf - buffer into which data can be written
     *  @param[in] msg - message to be encoded
     */
    libspdm_return_t encode(uint8_t eid, std::vector<uint8_t>& buf,
                            std::vector<uint8_t>& msg)
    {
        const size_t headerSize = sizeof(HeaderType);
        buf.resize(headerSize + msg.size());
        auto& header = getHeaderRef<HeaderType>(buf);
        header.mctpTag(MCTP_TAG_OWNER);
        header.eid = eid;
        std::copy(msg.begin(), msg.end(), buf.begin() + headerSize);

        return LIBSPDM_STATUS_SUCCESS;
    }

    /** @brief function called by ConnectionClass when decoding a received spdm
     * message
     *  @details it should analyze the transport data which starts at
     * buf[lay.getOffset] for correctness and set lay.Size appropriately
     * (lay.getEndOffset() must indicate where the spdm message begins)
     *  @param[in] buf - buffer containing the full received data
     *  @param[inout] lay - lay.Offset specifies where the transport layer
     * starts, lay.Size should be set to the size of the transport data
     */
    libspdm_return_t decode(uint8_t eid, std::vector<uint8_t>& buf,
                            void** message, size_t* message_size)
    {
        const auto& header = getHeaderRef<HeaderType>(buf);
        const size_t headerSize = sizeof(HeaderType);
        if (header.eid != eid)
        {
            return LIBSPDM_STATUS_INVALID_MSG_FIELD;
        }
        if (header.mctpTO())
        {
            return LIBSPDM_STATUS_INVALID_MSG_FIELD;
        }
        if (buf.size() <= headerSize)
        {
            lg2::error("Buffer too small to contain SPDM message");
            return LIBSPDM_STATUS_BUFFER_TOO_SMALL;
        }

        *message_size = buf.size() - headerSize;
        *message = malloc(*message_size);
        if (*message == nullptr)
        {
            lg2::error("Failed to allocate memory for SPDM message");
            return LIBSPDM_STATUS_BUFFER_FULL;
        }

        std::memcpy(*message, buf.data() + headerSize, *message_size);
        return LIBSPDM_STATUS_SUCCESS;
    }

  protected:
    /** @brief function to help with writing simple statically sized headers
     * into buf
     */
    template <class T>
    static T& getHeaderRef(std::vector<uint8_t>& buf)
    {
        return *reinterpret_cast<T*>(buf.data());
    }

    /** @brief Transport header matching the mctp-demux-daemon requirements
     */
    struct HeaderType
    {
        /** @brief MCTP header data
         */
        uint8_t mctpHeader;

        /** @brief Either source or the destination EndpointID, depending on
         * whether the packet is being sent or received. Regandless though it
         * should always
         */
        uint8_t eid;

        /** @brief Get The MCTP tag type
         */
        auto mctpTag() const noexcept -> uint8_t
        {
            return static_cast<uint8_t>(mctpHeader & 0x07);
        }

        /** @brief Set MCTP header to specific tag*/
        void mctpTag(uint8_t tag) noexcept
        {
            mctpHeader = static_cast<uint8_t>(tag) | 0x08U;
        }

        /** @brieg Get MCTO TO bit
         */
        auto mctpTO() const noexcept -> bool
        {
            return mctpHeader & 0x08;
        }
    };
};

// NOLINTNEXTLINE cppcoreguidelines-special-member-functions
class MctpIoClass : public IOClass
{
  public:
    explicit MctpIoClass() {}

    ~MctpIoClass() override
    {
        if (isSocketOpen())
        {
            deleteSocket();
        }
    }

    /**
     * @brief Creates a socket for MCTP communication in in-kernel mode.
     *
     * This function creates a socket using the MCTP protocol and binds it to
     * a specified address. If the socket creation or binding fails, it logs
     * the error and returns false.
     *
     * @return true if the socket is successfully created and bound, false
     * otherwise.
     */

    bool createSocket()
    {
        if (isSocketOpen())
        {
            return true;
        }
        // NOLINTNEXTLINE cppcoreguidelines-avoid-c-arrays
        struct sockaddr_mctp addr;

        Socket = socket(AF_MCTP, SOCK_DGRAM, 0);
        if (Socket < 0)
        {
            lg2::info("socket() error: {ERRNO}", "ERRNO", errno);
            return false;
        }

        // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-array-to-pointer-decay
        memset(&addr, 0, sizeof(addr));
        addr.smctp_family = AF_MCTP;
        addr.smctp_network = MCTP_NET_ANY;
        addr.smctp_addr.s_addr = MCTP_ADDR_ANY;
        addr.smctp_type = MCTP_TYPE_SPDM;
        addr.smctp_tag = MCTP_TAG_OWNER;

        int rc = bind(Socket, (struct sockaddr*)&addr, sizeof(addr));
        // NOLINTNEXTLINE cppcoreguidelines-pro-type-cstyle-cast
        if (rc)
        {
            lg2::info("bind to socket failed");
            deleteSocket();
            return false;
        }
        else
        {
            lg2::info("Binding success\n");
        }
        return true;
    }

    /**
     * @brief Closes the socket and resets the socket descriptor.
     *
     * This function closes the socket if it is open and resets the socket
     * descriptor to -1.
     */
    void deleteSocket()
    {
        close(Socket);
        Socket = -1;
    }

    libspdm_return_t write(const std::vector<uint8_t>& buf,
                           timeout_us_t timeout = timeoutUsInfinite) override;
    libspdm_return_t read(std::vector<uint8_t>& buf,
                          timeout_us_t timeout = timeoutUsInfinite) override;

    int isSocketOpen() const
    {
        return Socket != -1;
    }
    int getSocket() const
    {
        return Socket;
    }

  private:
    int Socket = -1;
};

/**
 * @brief Sends data over the MCTP socket.
 *
 * This function sends the provided buffer over the MCTP socket to a specified
 * address. It constructs the destination address using the MCTP protocol and
 * sends the data using the `sendto` function. If an error occurs during
 * sending, it logs the error and returns an error status.
 *
 * @param buf The buffer containing the data to be sent.
 * @param timeout The timeout value for the operation (not used in this
 * implementation).
 * @return LIBSPDM_STATUS_SUCCESS if the data is sent successfully,
 * LIBSPDM_STATUS_SEND_FAIL otherwise.
 */
inline libspdm_return_t
    MctpIoClass::write(const std::vector<uint8_t>& buf,
                       [[maybe_unused]] timeout_us_t /*timeout*/)
{
    struct sockaddr_mctp addr;
    memset(&addr, 0, sizeof(addr));

    addr.smctp_family = AF_MCTP;
    addr.smctp_network = MCTP_NET_ANY;
    addr.smctp_addr.s_addr = buf[1];
    addr.smctp_tag = MCTP_TAG_OWNER;
    addr.smctp_type = MCTP_TYPE_SPDM;

    ssize_t rc = sendto(Socket, buf.data() + 3, buf.size() - 3, 0,
                        reinterpret_cast<struct sockaddr*>(&addr),
                        sizeof(addr));
    if (rc == -1)
    {
        lg2::info("Send error: {ERRNO}", "ERRNO", errno);
        return LIBSPDM_STATUS_SEND_FAIL;
    }
    return LIBSPDM_STATUS_SUCCESS;
}

/**
 * @brief Reads data from the MCTP socket.
 *
 * This function reads data from the MCTP socket into the provided buffer. It
 * first peeks the buffer length to determine the size of the incoming message,
 * then resizes the buffer accordingly and reads the data. The function also
 * constructs a header with specific MCTP address and message type information
 * and prepends it to the buffer. If an error occurs during the read operation,
 * it logs the error and returns an error status.
 *
 * @param buf The buffer to store the received data.
 * @param timeout The timeout value for the operation (not used in this
 * implementation).
 * @return LIBSPDM_STATUS_SUCCESS if the data is read successfully,
 * LIBSPDM_STATUS_RECEIVE_FAIL otherwise.
 */
inline libspdm_return_t
    MctpIoClass::read(std::vector<uint8_t>& buf,
                      [[maybe_unused]] timeout_us_t /*timeout*/)
{
    struct sockaddr_mctp addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));

    ssize_t bufLen = recv(Socket, NULL, 0, MSG_PEEK | MSG_TRUNC);
    if (bufLen < 0)
    {
        lg2::info("Error peeking buffer length: {ERRNO}", "ERRNO", errno);
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }
    buf.resize(bufLen);
    ssize_t ret = recvfrom(Socket, buf.data(), buf.size(), MSG_TRUNC,
                           (struct sockaddr*)&addr, &addrlen);
    std::vector<int> head = {1, addr.smctp_addr.s_addr,
                             static_cast<int>(MCTP_TYPE_SPDM)};

    buf.insert(buf.begin(), head.begin(), head.end());

    if (ret == -1)
    {
        lg2::info("Receive error: {ERRNO}", "ERRNO", errno);
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }
    return LIBSPDM_STATUS_SUCCESS;
}

} // namespace spdm
