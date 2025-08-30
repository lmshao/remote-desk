/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "service_manager.h"

namespace lmshao::remotedesk {

ServiceManager::ServiceManager() = default;

ServiceManager::~ServiceManager() = default;

void ServiceManager::Unregister(const std::string &descriptor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(descriptor);
    if (it != services_.end()) {
        if (it->second->is_running && it->second->instance) {
            it->second->instance->Stop();
        }
        services_.erase(it);
    }
}

std::vector<std::string> ServiceManager::GetAllServices() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(services_.size());

    for (const auto &pair : services_) {
        result.push_back(pair.first);
    }

    return result;
}

size_t ServiceManager::GetServiceCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return services_.size();
}

bool ServiceManager::StartService(const std::string &descriptor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(descriptor);
    if (it == services_.end()) {
        LOG_ERROR("Service not found: %s", descriptor.c_str());
        return false;
    }

    auto &info = it->second;
    if (info->is_running) {
        LOG_DEBUG("Service already running: %s", descriptor.c_str());
        return true; // Already running
    }

    if (!info->instance) {
        info->instance = info->creator();
        if (!info->instance) {
            LOG_ERROR("Failed to create service instance: %s", descriptor.c_str());
            return false;
        }
    }

    if (info->instance->Start()) {
        info->is_running = true;
        LOG_DEBUG("Service started: %s", descriptor.c_str());
        return true;
    }

    LOG_ERROR("Failed to start service: %s", descriptor.c_str());
    return false;
}

void ServiceManager::StopService(const std::string &descriptor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(descriptor);
    if (it != services_.end() && it->second->is_running && it->second->instance) {
        it->second->instance->Stop();
        it->second->is_running = false;
    }
}

bool ServiceManager::StartAllServices()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool all_success = true;

    for (auto &pair : services_) {
        auto &info = pair.second;
        if (!info->is_running) {
            LOG_DEBUG("Starting service: %s", info->descriptor.c_str());
            if (!info->instance) {
                info->instance = info->creator();
                if (!info->instance) {
                    all_success = false;
                    LOG_ERROR("Failed to create service instance: %s", info->descriptor.c_str());
                    continue;
                }
            }

            if (info->instance->Start()) {
                info->is_running = true;
            } else {
                all_success = false;
                LOG_ERROR("Failed to start service: %s", info->descriptor.c_str());
            }
        }
    }

    return all_success;
}

void ServiceManager::StopAllServices()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto &pair : services_) {
        auto &info = pair.second;
        if (info->is_running && info->instance) {
            info->instance->Stop();
            info->is_running = false;
        }
    }
}

bool ServiceManager::IsServiceRunning(const std::string &descriptor) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(descriptor);
    return it != services_.end() && it->second->is_running;
}

const ServiceInfo *ServiceManager::GetServiceInfo(const std::string &descriptor) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = services_.find(descriptor);
    return it != services_.end() ? it->second.get() : nullptr;
}

void ServiceManager::SetEventCallback(const ServiceEventHandler &callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    event_callback_ = callback;
    LOG_DEBUG("Event callback registered");
}

void ServiceManager::NotifyMainService(const ServiceMessage &message)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    if (event_callback_) {
        event_callback_(message);
    }
}

} // namespace lmshao::remotedesk