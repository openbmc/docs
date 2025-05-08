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

#include "spdm_dbus_responder.hpp"

namespace spdm
{
SPDMDBusResponder::SPDMDBusResponder(sdbusplus::bus::bus& bus,
                                     const std::string& deviceName,
                                     const std::string& inventoryPath) :
    m_deviceName(deviceName),
    m_inventoryPath(inventoryPath)
{
    std::string componentIntegrityPath =
        "/xyz/openbmc_project/ComponentIntegrity/" + deviceName;
    componentIntegrity =
        std::make_unique<ComponentIntegrity>(bus, componentIntegrityPath);

    std::string trustedComponentPath =
        "/xyz/openbmc_project/TrustedComponent/" + deviceName;
    trustedComponent = std::make_unique<TrustedComponent>(bus,
                                                          trustedComponentPath);
}

void SPDMDBusResponder::updateVersion(spdm_version_number_t version)
{
    componentIntegrity->updateVersion(version);
}

void SPDMDBusResponder::updateEnabled(bool enabled)
{
    componentIntegrity->updateEnabled(enabled);
}

} // namespace spdm
