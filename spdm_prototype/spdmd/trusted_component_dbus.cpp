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

#include "trusted_component_dbus.hpp"

#include <phosphor-logging/lg2.hpp>

#include <chrono>

namespace spdm
{

/**
 * @brief Construct a new SPDM Trusted Component
 * @param bus D-Bus connection
 * @param path Object path for this responder
 */
TrustedComponent::TrustedComponent(sdbusplus::bus::bus& bus,
                                   const std::string& path) :
    sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Inventory::Item::server::
            TrustedComponent>(bus, path.c_str()),
    m_path(path)
{}

/**
 * @brief Update trusted component type
 * @param type New trusted component type
 * @throws std::runtime_error on D-Bus errors
 */
void TrustedComponent::updateTrustedComponentType(const std::string& type)
{
    if (type == "Integrated")
    {
        trustedComponentType(
            sdbusplus::xyz::openbmc_project::Inventory::Item::server::
                TrustedComponent::ComponentAttachType::Integrated);
    }
    else if (type == "Discrete")
    {
        trustedComponentType(
            sdbusplus::xyz::openbmc_project::Inventory::Item::server::
                TrustedComponent::ComponentAttachType::Discrete);
    }
    else
    {
        lg2::error("Invalid trusted component type: {TYPE}", "TYPE", type);
        trustedComponentType(
            sdbusplus::xyz::openbmc_project::Inventory::Item::server::
                TrustedComponent::ComponentAttachType::Unknown);
    }
}

} // namespace spdm
