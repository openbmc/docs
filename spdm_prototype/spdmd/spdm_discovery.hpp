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
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace spdm
{

/**
 * @brief Information about a discovered SPDM responder
 * @details Contains identification and connection information for an SPDM
 * device
 */
struct ResponderInfo
{
    size_t eid;             ///< Endpoint ID
    std::string objectPath; ///< D-Bus object path
    std::string uuid;       ///< Device UUID
};

/**
 * @brief Supported transport types for SPDM
 * @details Enumerates the different transport protocols that can be used
 *          for SPDM communication
 */
enum class TransportType
{
    MCTP,     ///< Management Component Transport Protocol
    PCIE_DOE, ///< PCIe Data Object Exchange
    TCP       ///< TCP/IP Protocol
};

/**
 * @brief Interface for SPDM transport protocols
 * @details Abstract base class defining the interface that all transport
 *          implementations must provide
 */
class DiscoveryProtocol
{
  public:
    virtual ~DiscoveryProtocol() = default;

    /**
     * @brief Discover SPDM-capable devices on this transport
     * @return Vector of discovered device information
     * @throws std::runtime_error on discovery failure
     */
    virtual std::vector<ResponderInfo> discoverDevices() = 0;

    /**
     * @brief Get the transport type
     * @return Transport type identifier
     */
    virtual TransportType getType() const = 0;
};

/**
 * @brief Main SPDM device discovery class
 * @details Manages the discovery of SPDM devices using a configured transport
 */
class SPDMDiscovery
{
  public:
    /**
     * @brief Construct a new SPDM Discovery object
     * @param transport Unique pointer to transport implementation
     */
    explicit SPDMDiscovery(
        std::unique_ptr<DiscoveryProtocol> discoveryProtocolIn);

    /**
     * @brief Start device discovery
     * @return true if devices were found, false otherwise
     */
    bool discover();

    /**
     * @brief Get discovered responder information
     * @return Const reference to vector of responder information
     */
    const auto& getRespondersInfo() const noexcept
    {
        return respInfos;
    }

  private:
    std::unique_ptr<DiscoveryProtocol>
        discoveryProtocol;                ///< Transport implementation
    std::vector<ResponderInfo> respInfos; ///< Discovered devices
};

} // namespace spdm
