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

#include "component_integrity_dbus.hpp"
#include "trusted_component_dbus.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/server/object.hpp>

namespace spdm
{

class SPDMDBusResponder
{
  public:
    /** @brief Default constructor is deleted */
    SPDMDBusResponder() = delete;

    /** @brief Copy constructor is deleted */
    SPDMDBusResponder(const SPDMDBusResponder&) = delete;

    /** @brief Assignment operator is deleted */
    SPDMDBusResponder& operator=(const SPDMDBusResponder&) = delete;

    /** @brief Move constructor is deleted */
    SPDMDBusResponder(SPDMDBusResponder&&) = delete;

    /** @brief Move assignment operator is deleted */
    SPDMDBusResponder& operator=(SPDMDBusResponder&&) = delete;

    /**
     * @brief Construct a new SPDM DBus Responder
     * @param bus D-Bus connection
     * @param deviceName Object path for this responder
     * @param eid MCTP Endpoint ID
     * @param inventoryPath Associated inventory object path
     */
    SPDMDBusResponder(sdbusplus::bus::bus& bus, const std::string& deviceName,
                      const std::string& inventoryPath);

    /**
     * @brief Virtual destructor
     */
    virtual ~SPDMDBusResponder() = default;

    /**
     * @brief Get the inventory object path
     * @return D-Bus object path for inventory
     */
    const std::string& getInventoryPath() const
    {
        return m_inventoryPath;
    }

    /**
     * @brief Update the SPDM version
     * @param version SPDM version
     */
    void updateVersion(spdm_version_number_t version);

    /**
     * @brief Update the enabled status
     * @param enabled New enabled status
     */
    void updateEnabled(bool enabled);

  private:
    /** @brief Device name */
    std::string m_deviceName;

    /** @brief Associated inventory object path */
    std::string m_inventoryPath;

    std::unique_ptr<ComponentIntegrity> componentIntegrity;
    std::unique_ptr<TrustedComponent> trustedComponent;
};

} // namespace spdm
