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

#include "mctp_helper.hpp"
#include "spdm_discovery.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

namespace spdm
{

/**
 * @brief MCTP-specific transport implementation
 * @details Handles discovery of SPDM devices over MCTP transport using D-Bus
 */
class MCTPDiscovery : public DiscoveryProtocol
{
  public:
    /**
     * @brief Construct a new MCTP Transport object
     * @param busRef Reference to D-Bus connection
     */
    explicit MCTPDiscovery(sdbusplus::bus::bus& busRef);

    /**
     * @brief Discover SPDM devices over MCTP
     * @return Vector of discovered device information
     * @throws sdbusplus::exception::SdBusError on D-Bus errors
     */
    std::vector<ResponderInfo> discoverDevices() override;

    /**
     * @brief Get the transport type
     * @return TransportType::MCTP
     */
    TransportType getType() const override
    {
        return TransportType::MCTP;
    }

  private:
    MctpIoClass mctpIo;
    /**
     * @brief Get list of MCTP services from D-Bus
     * @return Vector of pairs containing object path and service name
     * @throws sdbusplus::exception::SdBusError on D-Bus errors
     */
    std::vector<std::pair<std::string, std::string>> getMCTPServices();

    /**
     * @brief Get EID for a specific MCTP object path
     * @param objectPath D-Bus object path
     * @return EID value or invalid_eid if not found
     */
    size_t getEID(const std::string& objectPath, const std::string& service);

    /**
     * @brief Get UUID for a specific MCTP object path
     * @param objectPath D-Bus object path
     * @return UUID string or empty if not found
     */
    std::string getUUID(const std::string& objectPath,
                        const std::string& service);

    /**
     * @brief Get supported message types for an MCTP endpoint
     * @param objectPath D-Bus object path
     * @return Optional vector of supported message types
     */
    std::optional<std::vector<uint8_t>>
        getSupportedMessageTypes(const std::string& objectPath,
                                 const std::string& service);

    /**
     * @brief Get the transport socket for an MCTP endpoint
     * @param objectPath D-Bus object path
     * @param service D-Bus service name
     * @return Transport socket address or empty string if not found
     */
    std::string getTransportSocket(const std::string& objectPath,
                                   const std::string& service);
    sdbusplus::bus::bus& bus;                  ///< D-Bus connection
    static constexpr size_t invalid_eid = 255; ///< Invalid EID marker
    /// MCTP endpoint interface name
    static constexpr auto mctpEndpointIntfName =
        "xyz.openbmc_project.MCTP.Endpoint";

    /// UUID interface name
    static constexpr auto uuidIntfName = "xyz.openbmc_project.Common.UUID";

    static constexpr auto mctpPath = "/au/com/codeconstruct/mctp1";
};

} // namespace spdm
