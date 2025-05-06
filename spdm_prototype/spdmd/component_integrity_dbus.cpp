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

#include "component_integrity_dbus.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>

namespace spdm
{

/**
 * @brief Convert hashing algorithm to string representation
 * @param algo Algorithm enumeration value
 * @return String representation of algorithm
 */
std::string ComponentIntegrity::getHashingAlgorithmStr(uint16_t algo)
{
    switch (algo)
    {
        case SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_256:
            return "SHA256";
        case SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_384:
            return "SHA384";
        case SPDM_ALGORITHMS_BASE_HASH_ALGO_TPM_ALG_SHA_512:
            return "SHA512";
        default:
            return "NONE";
    }
}

/**
 * @brief Convert signing algorithm to string representation
 * @param algo Algorithm enumeration value
 * @return String representation of algorithm
 */
std::string ComponentIntegrity::getSigningAlgorithmStr(uint16_t algo)
{
    switch (algo)
    {
        case SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSASSA_2048:
            return "RSASSA2048";
        case SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_RSAPSS_2048:
            return "RSAPSS2048";
        case SPDM_ALGORITHMS_BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256:
            return "ECDSA_P256";
        default:
            return "NONE";
    }
}

/**
 * @brief Construct a new SPDM Responder
 * @param bus D-Bus connection
 * @param path Object path for this responder
 */
ComponentIntegrity::ComponentIntegrity(sdbusplus::bus::bus& bus,
                                       const std::string& path) :
    sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Attestation::server::
            ComponentIntegrity,
        sdbusplus::xyz::openbmc_project::Attestation::server::
            IdentityAuthentication,
        sdbusplus::xyz::openbmc_project::Attestation::server::MeasurementSet>(
        bus, path.c_str()),
    m_path(path)
{
    initializeProperties();
    initializeAuthProperties();
    initializeMeasurementProperties();
}

/**
 * @brief Initialize ComponentIntegrity properties
 * @details Sets initial values for all ComponentIntegrity interface properties
 */
void ComponentIntegrity::initializeProperties()
{
    // Initialize with empty version
    typeVersion("");

    // Enable component integrity checking
    enabled = true;

    // Initialize timestamps
    updateLastUpdateTime();

    // Initialize with empty measurements
    measurementsHash = {};
    measurementsSignature = {};
}

/**
 * @brief Initialize IdentityAuthentication properties
 * @details Sets initial values for all IdentityAuthentication interface
 * properties
 */
void ComponentIntegrity::initializeAuthProperties()
{
    using IdentityAuth = sdbusplus::xyz::openbmc_project::Attestation::server::
        IdentityAuthentication;

    // Initialize with unknown verification status
    verificationStatus = IdentityAuth::VerificationStatus::Unknown;

    // Initialize with no algorithms
    hashAlgo = 0;
    signAlgo = 0;

    // Initialize with empty certificates
    certificates = {};

    // Initialize capabilities
    capabilities = 0;
}

/**
 * @brief Initialize MeasurementSet properties
 * @details Sets initial values for all MeasurementSet interface properties
 */
void ComponentIntegrity::initializeMeasurementProperties()
{
    measurements = {};
    measurementsHash = {};
    // measurementsSignature = {};
    nonce = {};
}

/**
 * @brief Update measurement data
 * @param newMeasurements New measurement hash
 * @param newSignature New measurement signature
 * @throws std::runtime_error on D-Bus errors
 */
void ComponentIntegrity::updateMeasurements(
    const std::vector<uint8_t>& newMeasurements,
    const std::vector<uint8_t>& newSignature)
{
    try
    {
        // Update measurement properties
        measurementsHash = newMeasurements;
        measurementsSignature = newSignature;

        // Update timestamp
        updateLastUpdateTime();

        lg2::info("Updated measurements for SPDM responder path {OBJ_PATH}",
                  "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to update measurements for path {OBJ_PATH}: {ERROR}",
                   "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateVersion(const std::string& newVersion)
{
    try
    {
        version = newVersion;

        lg2::info("Updated SPDM version to {VERSION} for path {OBJ_PATH}",
                  "VERSION", newVersion, "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to update version for path {OBJ_PATH}: {ERROR}",
                   "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateCertificate(uint8_t slot,
                                           const std::string& certData)
{
    try
    {
        certificates.emplace_back(slot, certData);

        lg2::info("Updated certificate for slot {SLOT}, path {OBJ_PATH}",
                  "SLOT", slot, "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to update certificate for path {OBJ_PATH}: {ERROR}",
                   "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateVerificationStatus(VerificationStatus status)
{
    try
    {
        verificationStatus = status;

        lg2::info("Updated verification status to {STATUS} for path {OBJ_PATH}",
                  "STATUS", static_cast<int>(status), "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error(
            "Failed to update verification status for path {OBJ_PATH}: {ERROR}",
            "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateAlgorithms(uint16_t newHashAlgo,
                                          uint16_t newSignAlgo)
{
    try
    {
        hashAlgo = newHashAlgo;
        signAlgo = newSignAlgo;

        lg2::info("Updated algorithms for path {OBJ_PATH}", "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to update algorithms for path {OBJ_PATH}: {ERROR}",
                   "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateCapabilities(uint32_t caps)
{
    try
    {
        capabilities = caps;

        lg2::info("Updated capabilities to {CAPS} for path {OBJ_PATH}", "CAPS",
                  caps, "OBJ_PATH", m_path);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to update capabilities for path {OBJ_PATH}: {ERROR}",
                   "OBJ_PATH", m_path, "ERROR", e.what());
        throw;
    }
}

void ComponentIntegrity::updateLastUpdateTime()
{
    auto now = std::chrono::system_clock::now();
    auto timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    lastUpdated(timestamp);
}

std::tuple<sdbusplus::message::object_path, std::string, std::string,
           std::string, std::string, std::string>
    ComponentIntegrity::spdmGetSignedMeasurements(
        std::vector<size_t> measurementIndices, std::string nonce,
        size_t slotId)
{
    using namespace sdbusplus::xyz::openbmc_project::Common::Error;

    // Validate nonce length (32 bytes hex encoded = 64 characters)
    if (nonce.length() != 64)
    {
        lg2::error("Invalid nonce length: {LENGTH}", "LENGTH", nonce.length());
        throw InvalidArgument();
    }

    // Validate slot ID
    if (slotId >= 8) // Assuming max 8 slots
    {
        lg2::error("Invalid slot ID: {SLOT}", "SLOT", slotId);
        throw InvalidArgument();
    }

    // Validate measurement indices
    for (const auto& idx : measurementIndices)
    {
        if (idx >= 255)
        {
            lg2::error("Invalid measurement index: {INDEX}", "INDEX", idx);
            throw InvalidArgument();
        }
    }

    try
    {
        // Get current certificate path
        auto certPath = sdbusplus::message::object_path(
            "/xyz/openbmc_project/certs/spdm/slot" + std::to_string(slotId));

        // Get current algorithms
        auto hashAlgoStr = getHashingAlgorithmStr(hashAlgo);
        auto signAlgoStr = getSigningAlgorithmStr(signAlgo);

        // Get current version
        auto version = typeVersion();

        // Get public key and signed measurements
        // Note: In a real implementation, these would come from the SPDM device
        std::string pubKey = "";     // Get from device
        std::string signedMeas = ""; // Get from device

        lg2::info("Got signed measurements for path {OBJ_PATH}, slot {SLOT}",
                  "OBJ_PATH", m_path, "SLOT", slotId);

        return std::make_tuple(std::move(certPath), hashAlgoStr, pubKey,
                               signedMeas, signAlgoStr, version);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get signed measurements: {ERROR}", "ERROR",
                   e.what());
        throw InternalFailure();
    }
}

} // namespace spdm
