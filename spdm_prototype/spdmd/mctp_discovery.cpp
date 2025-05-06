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

#include "mctp_discovery.hpp"

namespace spdm
{

/**
 * @brief Constructs MCTP transport object
 * @param busRef Reference to D-Bus connection
 */
MCTPDiscovery::MCTPDiscovery(sdbusplus::bus::bus& busRef) : bus(busRef) {}

/**
 * @brief Discovers SPDM devices over MCTP
 * @details Queries D-Bus for MCTP endpoints that support SPDM
 *
 * @return Vector of discovered SPDM devices
 * @throws sdbusplus::exception::SdBusError on D-Bus communication errors
 */
std::vector<ResponderInfo> MCTPDiscovery::discoverDevices()
{
    std::vector<ResponderInfo> devices;
    try
    {
        auto services = getMCTPServices();
        for (const auto& [objectPath, service] : services)
        {
            auto messageTypes = getSupportedMessageTypes(objectPath, service);
            if (!messageTypes || 
                std::find(messageTypes->begin(), messageTypes->end(),
                         MCTP_MESSAGE_TYPE_SPDM) == messageTypes->end())
            {
                lg2::debug("Endpoint does not support SPDM: {PATH}", 
                          "PATH", objectPath);
                continue;
            }

            auto eid = getEID(objectPath, service);
            if (eid == invalid_eid)
            {
                lg2::error("Invalid EID for object: {PATH}", "PATH", objectPath);
                continue;
            }
            auto uuid = getUUID(objectPath, service);
            if (uuid.empty())
            {
                lg2::error("Failed to get UUID for object: {PATH}", "PATH", objectPath);
                continue;
            }
            ResponderInfo info{
                eid,
                objectPath,
                uuid
            };
            
            devices.emplace_back(std::move(info));
            lg2::info("Found SPDM device: {PATH}", "PATH", objectPath);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("MCTP device discovery error: {ERROR}", "ERROR", e.what());
    }
    return devices;
}

/**
 * @brief Gets list of MCTP services from D-Bus
 * @details Queries ObjectMapper for services implementing MCTP endpoint interface
 *
 * @return Vector of pairs containing object path and service name
 * @throws sdbusplus::exception::SdBusError on D-Bus errors
 */
std::vector<std::pair<std::string, std::string>> 
MCTPDiscovery::getMCTPServices()
{
    std::vector<std::pair<std::string, std::string>> services;
    try
    {
        auto mapper = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper",
            "GetSubTree");

        mapper.append(mctpPath, 0, 
                     std::array<std::string, 1>{mctpEndpointIntfName});

        std::map<std::string, std::map<std::string, std::vector<std::string>>> subtree;
        auto reply = bus.call(mapper);
        reply.read(subtree);

        for (const auto& [path, serviceMap] : subtree)
        {
            if (!serviceMap.empty())
            {
                services.emplace_back(path, serviceMap.begin()->first);
            }
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Failed to get MCTP services: {ERROR}", "ERROR", e.what());
        throw;
    }
    return services;
}

/**
 * @brief Gets EID for a specific MCTP object path
 * @details Queries D-Bus properties interface for EID value
 *
 * @param objectPath D-Bus object path to query
 * @return EID value or invalid_eid if not found
 */
size_t MCTPDiscovery::getEID(const std::string& objectPath, const std::string& service)
{
    try
    {
        auto method = bus.new_method_call(
            service.c_str(),
            objectPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
        
        method.append(mctpEndpointIntfName, "EID");
        
        auto reply = bus.call(method);
        std::variant<uint8_t, uint32_t> value;
        reply.read(value);

        if (auto eid = std::get_if<uint8_t>(&value))
        {
            return *eid;
        }
        else if (auto eid32 = std::get_if<uint32_t>(&value))
        {
            return static_cast<size_t>(*eid32);
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get EID: {ERROR}", "ERROR", e.what());
    }
    return invalid_eid;
}

/**
 * @brief Gets UUID for a specific MCTP object path
 * @details Queries D-Bus properties interface for UUID value
 *
 * @param objectPath D-Bus object path to query
 * @return UUID string or empty if not found
 */
std::string MCTPDiscovery::getUUID(const std::string& objectPath, const std::string& service)
{
    try
    {
        auto method = bus.new_method_call(
            service.c_str(),
            objectPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
            
        method.append(uuidIntfName, "UUID");
        
        auto reply = bus.call(method);
        std::variant<std::string> uuid;
        reply.read(uuid);
        return std::get<std::string>(uuid);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get UUID: {ERROR}", "ERROR", e.what());
        return {};
    }
}

/**
 * @brief Get supported message types for an MCTP endpoint
 * @param objectPath D-Bus object path
 * @return Optional vector of supported message types
 */
std::optional<std::vector<uint8_t>> 
MCTPDiscovery::getSupportedMessageTypes(const std::string& objectPath, const std::string& service)
{
    lg2::error("DEBUG: getSupportedMessageTypes: {OBJECT_PATH} {SERVICE}", "OBJECT_PATH", objectPath, "SERVICE", service);
    try
    {
        auto method = bus.new_method_call(
            service.c_str(),
            objectPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
        
        method.append(mctpEndpointIntfName, "SupportedMessageTypes");
        
        auto reply = bus.call(method);
        std::variant<std::vector<uint8_t>> messageTypes;
        reply.read(messageTypes);
        return std::get<std::vector<uint8_t>>(messageTypes);
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get message types: {ERROR}", "ERROR", e.what());
        return std::nullopt;
    }
}

std::string MCTPDiscovery::getTransportSocket(const std::string& objectPath, const std::string& service)
{
    try
    {
        auto method = bus.new_method_call(
            service.c_str(),
            objectPath.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get");
        
        method.append("xyz.openbmc_project.Common.UnixSocket", "Address");
        
        auto reply = bus.call(method);
        std::variant<std::vector<uint8_t>> address;
        reply.read(address);
        try {
            auto& vec = std::get<std::vector<uint8_t>>(address);
            if (vec.empty()) {
                lg2::error("Transport socket address is empty");
                return {};
            }
            return {vec.begin(), vec.end()};
        } catch (const std::bad_variant_access& e) {
            lg2::error("Invalid transport socket address format: {ERROR}", "ERROR", e.what());
            return {};
        }
    }
    catch (const std::exception& e)
    {
        lg2::error("Failed to get message types: {ERROR}", "ERROR", e.what());
        return {};
    }
}

} // namespace spdm
