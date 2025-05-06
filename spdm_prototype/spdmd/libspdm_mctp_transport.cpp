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

#include "libspdm_mctp_transport.hpp"

#include <phosphor-logging/lg2.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace spdm
{

bool SpdmMctpTransport::initialize()
{
    m_spdm_context = (void*)malloc(libspdm_get_context_size());
    if (!m_spdm_context)
    {
        lg2::error("Failed to allocate SPDM context");
        return false;
    }
    libspdm_return_t status = libspdm_init_context(m_spdm_context);
    if (!m_spdm_context || status != LIBSPDM_STATUS_SUCCESS)
    {
        lg2::error("Failed to initialize SPDM context");
        return false;
    }
    m_scratch_buffer = malloc(MAX_MCTP_PACKET_SIZE);
    if (!m_scratch_buffer)
    {
        lg2::error("Failed to allocate scratch buffer");
        libspdm_deinit_context(m_spdm_context);
        m_spdm_context = nullptr;
        return false;
    }
    libspdm_context_t* context =
        static_cast<libspdm_context_t*>(m_spdm_context);
    context->app_context_data_ptr = this;
    libspdm_register_device_io_func(m_spdm_context,
                                    &SpdmMctpTransport::device_send_message,
                                    &SpdmMctpTransport::device_receive_message);
    libspdm_register_transport_layer_func(
        m_spdm_context, LIBSPDM_MAX_SPDM_MSG_SIZE,
        LIBSPDM_TRANSPORT_HEADER_SIZE, LIBSPDM_TRANSPORT_TAIL_SIZE,
        libspdm_transport_mctp_encode_message,
        libspdm_transport_mctp_decode_message);
    libspdm_register_device_buffer_func(
        m_spdm_context, LIBSPDM_SENDER_BUFFER_SIZE,
        LIBSPDM_RECEIVER_BUFFER_SIZE,
        &SpdmMctpTransport::spdm_device_acquire_sender_buffer,
        &SpdmMctpTransport::spdm_device_release_sender_buffer,
        &SpdmMctpTransport::spdm_device_acquire_receiver_buffer,
        &SpdmMctpTransport::spdm_device_release_receiver_buffer);
    size_t scratch_buffer_size =
        libspdm_get_sizeof_required_scratch_buffer(m_spdm_context);
    m_scratch_buffer = malloc(scratch_buffer_size);
    if (m_scratch_buffer == nullptr)
    {
        libspdm_deinit_context(m_spdm_context);
        return false;
    }
    libspdm_set_scratch_buffer(m_spdm_context, m_scratch_buffer,
                               scratch_buffer_size);
    if (!libspdm_check_context(m_spdm_context))
    {
        lg2::error("Context check failed");
        free(m_scratch_buffer);
        m_scratch_buffer = nullptr;
        libspdm_deinit_context(m_spdm_context);
        return false;
    }
    if (m_use_version != 0)
    {
        spdm_version_number_t spdm_version;
        libspdm_zero_mem(&parameter, sizeof(parameter));
        parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;
        spdm_version = m_use_version << SPDM_VERSION_NUMBER_SHIFT_BIT;
        libspdm_set_data(m_spdm_context, LIBSPDM_DATA_SPDM_VERSION, &parameter,
                         &spdm_version, sizeof(spdm_version));
    }
    libspdm_zero_mem(&parameter, sizeof(parameter));
    parameter.location = LIBSPDM_DATA_LOCATION_LOCAL;

    uint8_t data8 = 0;
    libspdm_set_data(m_spdm_context, LIBSPDM_DATA_CAPABILITY_CT_EXPONENT,
                     &parameter, &data8, sizeof(data8));

    data8 = m_support_measurement_spec;
    libspdm_set_data(m_spdm_context, LIBSPDM_DATA_MEASUREMENT_SPEC, &parameter,
                     &data8, sizeof(data8));
    uint32_t data32 = m_support_asym_algo;
    libspdm_set_data(m_spdm_context, LIBSPDM_DATA_BASE_ASYM_ALGO, &parameter,
                     &data32, sizeof(data32));
    data32 = m_support_hash_algo;
    libspdm_set_data(m_spdm_context, LIBSPDM_DATA_BASE_HASH_ALGO, &parameter,
                     &data32, sizeof(data32));

    return true;
}

libspdm_return_t SpdmMctpTransport::device_send_message(void* spdm_context,
                                                        size_t message_size,
                                                        const void* message,
                                                        uint64_t timeout)
{
    try
    {
        libspdm_context_t* context =
            static_cast<libspdm_context_t*>(spdm_context);
        auto transport =
            static_cast<SpdmMctpTransport*>(context->app_context_data_ptr);
        if (!transport)
        {
            lg2::error("SpdmMctpTransport instance is nullptr");
            return LIBSPDM_STATUS_INVALID_STATE_LOCAL;
        }
        std::vector<uint8_t> msg(static_cast<const uint8_t*>(message),
                                 static_cast<const uint8_t*>(message) +
                                     message_size);
        timeout_us_t timeoutUs = static_cast<timeout_us_t>(timeout);
        std::vector<uint8_t> mctpMessage;
        if (transport->m_mctpMessageTransport.encode(
                transport->m_eid, mctpMessage, msg) != LIBSPDM_STATUS_SUCCESS)
        {
            lg2::error("Failed to encode MCTP message");
            return LIBSPDM_STATUS_SEND_FAIL;
        }
        libspdm_return_t ret = transport->m_mctpIo.write(mctpMessage,
                                                         timeoutUs);
        if (ret != LIBSPDM_STATUS_SUCCESS)
        {
            lg2::error("Failed to send SPDM message");
            return LIBSPDM_STATUS_SEND_FAIL;
        }
        return LIBSPDM_STATUS_SUCCESS;
    }
    catch (const std::exception&)
    {
        return LIBSPDM_STATUS_SEND_FAIL;
    }
}

libspdm_return_t SpdmMctpTransport::device_receive_message(void* spdm_context,
                                                           size_t* message_size,
                                                           void** message,
                                                           uint64_t timeout)
{
    try
    {
        libspdm_context_t* context =
            static_cast<libspdm_context_t*>(spdm_context);
        auto transport =
            static_cast<SpdmMctpTransport*>(context->app_context_data_ptr);
        if (!transport)
        {
            return LIBSPDM_STATUS_INVALID_STATE_LOCAL;
        }
        timeout_us_t timeoutUs = static_cast<timeout_us_t>(timeout);
        std::vector<uint8_t> response;
        libspdm_return_t ret = transport->m_mctpIo.read(response, timeoutUs);
        if (ret != LIBSPDM_STATUS_SUCCESS)
        {
            lg2::error("Failed to receive SPDM message");
            return LIBSPDM_STATUS_RECEIVE_FAIL;
        }
        if (transport->m_mctpMessageTransport.decode(transport->m_eid, response,
                                                     message, message_size) !=
            LIBSPDM_STATUS_SUCCESS)
        {
            lg2::error("Failed to decode MCTP message");
            return LIBSPDM_STATUS_RECEIVE_FAIL;
        }
        return LIBSPDM_STATUS_SUCCESS;
    }
    catch (const std::exception&)
    {
        return LIBSPDM_STATUS_RECEIVE_FAIL;
    }
}

libspdm_return_t
    SpdmMctpTransport::spdm_device_acquire_sender_buffer(void* context,
                                                         void** msg_buf_ptr)
{
    libspdm_context_t* spdm_context = (libspdm_context_t*)context;
    SpdmMctpTransport* transport =
        static_cast<SpdmMctpTransport*>(spdm_context->app_context_data_ptr);

    if (transport->m_send_receive_buffer_acquired)
    {
        return LIBSPDM_STATUS_INVALID_STATE_LOCAL;
    }
    *msg_buf_ptr = transport->m_send_receive_buffer;
    libspdm_zero_mem(transport->m_send_receive_buffer,
                     sizeof(transport->m_send_receive_buffer));
    transport->m_send_receive_buffer_acquired = true;

    return LIBSPDM_STATUS_SUCCESS;
}

void SpdmMctpTransport::spdm_device_release_sender_buffer(
    void* context, const void* /*msg_buf_ptr*/)
{
    libspdm_context_t* spdm_context = (libspdm_context_t*)context;
    SpdmMctpTransport* transport =
        static_cast<SpdmMctpTransport*>(spdm_context->app_context_data_ptr);
    transport->m_send_receive_buffer_acquired = false;
}

libspdm_return_t
    SpdmMctpTransport::spdm_device_acquire_receiver_buffer(void* context,
                                                           void** msg_buf_ptr)
{
    libspdm_context_t* spdm_context = (libspdm_context_t*)context;
    SpdmMctpTransport* transport =
        static_cast<SpdmMctpTransport*>(spdm_context->app_context_data_ptr);

    if (transport->m_send_receive_buffer_acquired)
    {
        return LIBSPDM_STATUS_INVALID_STATE_LOCAL;
    }

    *msg_buf_ptr = transport->m_send_receive_buffer;
    libspdm_zero_mem(transport->m_send_receive_buffer,
                     sizeof(transport->m_send_receive_buffer));
    transport->m_send_receive_buffer_acquired = true;

    return LIBSPDM_STATUS_SUCCESS;
}

void SpdmMctpTransport::spdm_device_release_receiver_buffer(
    void* context, const void* /*msg_buf_ptr*/)
{
    libspdm_context_t* spdm_context = (libspdm_context_t*)context;
    SpdmMctpTransport* transport =
        static_cast<SpdmMctpTransport*>(spdm_context->app_context_data_ptr);
    transport->m_send_receive_buffer_acquired = false;
}

} // namespace spdm
