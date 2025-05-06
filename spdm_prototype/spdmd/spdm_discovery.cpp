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

#include "spdm_discovery.hpp"

namespace spdm
{

/**
 * @brief Constructs the SPDM discovery object
 * @details Initializes discovery with provided transport implementation
 *
 * @param transportIn Unique pointer to transport implementation
 */
SPDMDiscovery::SPDMDiscovery(std::unique_ptr<DiscoveryProtocol> discoveryProtocolIn) :
    discoveryProtocol(std::move(discoveryProtocolIn))
{}

/**
 * @brief Performs device discovery
 * @details Initiates discovery process using configured transport
 *
 * @return true if devices were found, false otherwise
 * @throws std::runtime_error on discovery failure
 */
bool SPDMDiscovery::discover()
{
    try
    {
        respInfos = discoveryProtocol->discoverDevices();
        return !respInfos.empty();
    }
    catch (const std::exception& e)
    {
        lg2::error("Discovery failed: {ERROR}", "ERROR", e.what());
        return false;
    }
}

} // namespace spdm
