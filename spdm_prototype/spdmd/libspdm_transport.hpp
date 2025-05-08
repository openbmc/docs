
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
#include <phosphor-logging/lg2.hpp>

extern "C"
{
#include "internal/libspdm_common_lib.h"
#include "library/spdm_common_lib.h"
#include "library/spdm_requester_lib.h"
#include "library/spdm_return_status.h"
#include "library/spdm_transport_mctp_lib.h"
}

#define LIBSPDM_MAX_SPDM_MSG_SIZE 0x1200
#define LIBSPDM_TRANSPORT_HEADER_SIZE 64
#define LIBSPDM_TRANSPORT_TAIL_SIZE 64
#define LIBSPDM_TRANSPORT_ADDITIONAL_SIZE                                      \
    (LIBSPDM_TRANSPORT_HEADER_SIZE + LIBSPDM_TRANSPORT_TAIL_SIZE)
#ifndef LIBSPDM_SENDER_BUFFER_SIZE
#define LIBSPDM_SENDER_BUFFER_SIZE                                             \
    (LIBSPDM_MAX_SPDM_MSG_SIZE + LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif
#ifndef LIBSPDM_RECEIVER_BUFFER_SIZE
#define LIBSPDM_RECEIVER_BUFFER_SIZE                                           \
    (LIBSPDM_MAX_SPDM_MSG_SIZE + LIBSPDM_TRANSPORT_ADDITIONAL_SIZE)
#endif
#if (LIBSPDM_SENDER_BUFFER_SIZE > LIBSPDM_RECEIVER_BUFFER_SIZE)
#define LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE LIBSPDM_SENDER_BUFFER_SIZE
#else
#define LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE LIBSPDM_RECEIVER_BUFFER_SIZE
#endif

namespace spdm
{

class SpdmTransport
{
  public:
    SpdmTransport() : m_spdm_context(nullptr), m_scratch_buffer(nullptr) {}
    ~SpdmTransport()
    {
        if (m_scratch_buffer)
        {
            free(m_scratch_buffer);
            m_scratch_buffer = nullptr;
        }
        if (m_spdm_context)
        {
            libspdm_deinit_context(m_spdm_context);
        }
    }
    uint8_t m_send_receive_buffer[LIBSPDM_MAX_SENDER_RECEIVER_BUFFER_SIZE];
    bool m_send_receive_buffer_acquired = false;
    uint8_t m_use_version = SPDM_MESSAGE_VERSION_11;
    uint32_t m_use_requester_capability_flags = 0;
    uint8_t m_use_req_slot_id = 0xFF;
    uint32_t m_use_capability_flags = 0;
    uint32_t m_use_hash_algo;
    uint32_t m_use_measurement_hash_algo;
    uint32_t m_use_asym_algo;
    uint16_t m_use_req_asym_algo;

    uint8_t m_support_measurement_spec = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
    // uint8_t m_support_mel_spec;
    uint32_t m_support_measurement_hash_algo =
        SPDM_ALGORITHMS_MEASUREMENT_HASH_ALGO_TPM_ALG_SHA_384 |
        SPDM_ALGORITHMS_MEASUREMENT_HASH_ALGO_TPM_ALG_SHA_512 |
        SPDM_ALGORITHMS_MEASUREMENT_HASH_ALGO_TPM_ALG_SHA_256;
    // uint32_t m_support_hash_algo =
    // SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384 |
    // SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_512 |
    // SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256;
    uint32_t m_support_hash_algo =
        SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256 |
        SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384 |
        SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_512;
    // uint32_t m_support_asym_algo =
    // SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_2048;
    uint32_t m_support_asym_algo =
        SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256 |
        SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
        SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P521;
    uint16_t m_support_req_asym_algo =
        SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_2048;
    uint16_t m_support_dhe_algo = SPDM_ALGORITHMS_DHE_NAMED_GROUP_SECP_384_R1;
    uint16_t m_support_aead_algo =
        SPDM_ALGORITHMS_AEAD_CIPHER_SUITE_AES_256_GCM;
    uint16_t m_support_key_schedule_algo =
        SPDM_ALGORITHMS_KEY_SCHEDULE_HMAC_HASH;
    uint8_t m_support_other_params_support = 0;
    libspdm_data_parameter_t parameter;
    void* m_spdm_context;
    void* m_scratch_buffer;

    virtual bool initialize() = 0;

    bool doVcaNegotiation()
    {
        spdm_version_number_t spdm_responder_version;
        size_t data_size;
        if (m_spdm_context == nullptr)
        {
            lg2::error("Error: SPDM context is nullptr");
            return false;
        }
        libspdm_return_t status = libspdm_init_connection(m_spdm_context,
                                                          false);
        if (status != LIBSPDM_STATUS_SUCCESS)
        {
            lg2::error("Error: Failed to initialize SPDM connection");
            return false;
        }
        libspdm_zero_mem(&parameter, sizeof(parameter));
        parameter.location = LIBSPDM_DATA_LOCATION_CONNECTION;
        data_size = sizeof(spdm_responder_version);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_SPDM_VERSION, &parameter,
                         &spdm_responder_version, &data_size);
        spdm_responder_version = spdm_responder_version >>
                                 SPDM_VERSION_NUMBER_SHIFT_BIT;
        if (m_use_version == 0)
        {
            m_use_version = spdm_responder_version;
        }
        if (m_use_version != spdm_responder_version)
        {
            lg2::error(
                "Error: SPDM version mismatch - requester: {REQ_VERSION}, responder: {RESP_VERSION}",
                "REQ_VERSION", static_cast<int>(m_use_version), "RESP_VERSION",
                static_cast<int>(spdm_responder_version));
            return false;
        }
        uint32_t respCapabilityFlags;
        data_size = sizeof(respCapabilityFlags);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_CAPABILITY_FLAGS,
                         &parameter, &respCapabilityFlags, &data_size);
        // Check connection state
        uint32_t connection_state;
        data_size = sizeof(connection_state);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_CONNECTION_STATE,
                         &parameter, &connection_state, &data_size);

        // Get negotiated measurement hash algorithm
        uint32_t negotiated_measurement_hash_algo;
        data_size = sizeof(negotiated_measurement_hash_algo);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_MEASUREMENT_HASH_ALGO,
                         &parameter, &negotiated_measurement_hash_algo,
                         &data_size);
        // Get negotiated base hash algorithm
        uint32_t negotiated_hash_algo;
        data_size = sizeof(negotiated_hash_algo);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_BASE_HASH_ALGO,
                         &parameter, &negotiated_hash_algo, &data_size);
        lg2::info("SPDM negotiated base hash algorithm: {HASH_ALGO}",
                  "HASH_ALGO", negotiated_hash_algo);

        // Get negotiated asymmetric algorithm
        uint32_t negotiated_asym_algo;
        data_size = sizeof(negotiated_asym_algo);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_BASE_ASYM_ALGO,
                         &parameter, &negotiated_asym_algo, &data_size);

        // Get negotiated requester asymmetric algorithm
        uint16_t negotiated_req_asym_algo;
        data_size = sizeof(negotiated_req_asym_algo);
        libspdm_get_data(m_spdm_context, LIBSPDM_DATA_REQ_BASE_ASYM_ALG,
                         &parameter, &negotiated_req_asym_algo, &data_size);

        // Store the negotiated values in member variables
        m_use_measurement_hash_algo = negotiated_measurement_hash_algo;
        m_use_hash_algo = negotiated_hash_algo;
        m_use_asym_algo = negotiated_asym_algo;
        m_use_req_asym_algo = negotiated_req_asym_algo;
        return true;
    }
};

} // namespace spdm
