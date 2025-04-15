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

#include "spdmd_app.hpp"

#include <phosphor-logging/lg2.hpp>

namespace spdmd
{

SPDMDaemon::SPDMDaemon() :
    bus(sdbusplus::bus::new_default()), objManager(bus, spdmRootPath)
{
    bus.request_name(spdmBusName);
}

int SPDMDaemon::run()
{
    lg2::info("SPDM daemon starting");
    bus.process_loop();
    return EXIT_SUCCESS;
}

} // namespace spdmd
