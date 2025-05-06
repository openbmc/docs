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

#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Inventory/Item/TrustedComponent/server.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/server/object.hpp>

namespace spdm
{
/**
 * @class TrustedComponent
 * @brief SPDM Responder implementation with D-Bus interfaces
 * @details Implements TrustedComponent interface for SPDM device management.
 */
class TrustedComponent :
    public sdbusplus::server::object::object<
        sdbusplus::xyz::openbmc_project::Inventory::Item::server::
            TrustedComponent>
{
  public:
    /** @brief Default constructor is deleted */
    TrustedComponent() = delete;

    /** @brief Copy constructor is deleted */
    TrustedComponent(const TrustedComponent&) = delete;

    /** @brief Assignment operator is deleted */
    TrustedComponent& operator=(const TrustedComponent&) = delete;

    /** @brief Move constructor is deleted */
    TrustedComponent(TrustedComponent&&) = delete;

    /** @brief Move assignment operator is deleted */
    TrustedComponent& operator=(TrustedComponent&&) = delete;

    /**
     * @brief Construct a new SPDM DBus Responder
     * @param bus D-Bus connection
     * @param path Object path for this responder
     */
    TrustedComponent(sdbusplus::bus::bus& bus, const std::string& path);

    /**
     * @brief Virtual destructor
     */
    virtual ~TrustedComponent() = default;

    /**
     * @brief Update trusted component type
     * @param type New trusted component type
     * @throws std::runtime_error on D-Bus errors
     */
    void updateTrustedComponentType(const std::string& type);

  private:
    std::string m_path;
};

} // namespace spdm
