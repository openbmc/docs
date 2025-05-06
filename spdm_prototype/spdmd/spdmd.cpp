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
#include "mctp_discovery.hpp"
#include "spdm_dbus_responder.hpp"
#include "spdm_discovery.hpp"
#include "spdmd_app.hpp"

#include <phosphor-logging/lg2.hpp>

#include <iostream>

// Main function must be in global namespace
int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    try
    {
        auto app = spdmd::SPDMDaemon();

        //---- MCTP Discovery------//
        // Get D-Bus connection from app
        auto& bus = app.getBus();

        // Create MCTP transport
        auto mctpDiscovery = std::make_unique<spdm::MCTPDiscovery>(bus);

        // Create discovery instance
        spdm::SPDMDiscovery discovery(std::move(mctpDiscovery));

        // Perform discovery
        if (discovery.discover())
        {
            // Print discovered devices
            for (const auto& device : discovery.getRespondersInfo())
            {
                std::cout << "Found SPDM device:\n"
                          << "  Path: " << device.objectPath << "\n"
                          << "  EID: " << device.eid << "\n"
                          << "  UUID: " << device.uuid << "\n";
                auto initResult = device.transport->initialize();
                if (!initResult)
                {
                    lg2::error("Failed to initialize SPDM transport");
                    return EXIT_FAILURE;
                }
                auto vcaResult = device.transport->doVcaNegotiation();
                if (!vcaResult)
                {
                    lg2::error("Failed to perform SPDM VCA negotiation");
                    return EXIT_FAILURE;
                }
            }
        }
        else
        {
            lg2::error("No SPDM devices found");
        }
        return app.run();
    }
    catch (const std::exception& e)
    {
        lg2::error("Fatal error: {ERROR}", "ERROR", e.what());
        return EXIT_FAILURE;
    }
}
