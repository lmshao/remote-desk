/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_SERVICE_MANAGER_H
#define LMSHAO_REMOTE_DESK_SERVICE_MANAGER_H

#include <coreutils/singleton.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../log/remote_desk_log.h"

namespace lmshao::remotedesk {

using namespace lmshao::coreutils;

//==============================================================================
// Forward Declarations
//==============================================================================
template <typename T>
class ServiceDelegator;

//==============================================================================
// Registration Macro
//==============================================================================
#define REGISTER_SERVICE(ServiceClass, name)                                                                           \
public:                                                                                                                \
    static std::string GetName()                                                                                       \
    {                                                                                                                  \
        return name;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
private:                                                                                                               \
    static inline ServiceDelegator<ServiceClass> delegator_;

//==============================================================================
// Service Base Interface
//==============================================================================
class ServiceBase {
public:
    virtual ~ServiceBase() = default;

    /**
     * @brief Start service - MUST be non-blocking
     * @return true if started successfully, false otherwise
     * Requirements:
     * 1. Complete initialization quickly (usually < 100ms)
     * 2. Start background threads if long-running work is needed
     * 3. Service should be ready when this returns
     * 4. Support repeated calls (idempotent)
     */
    virtual bool Start() = 0;

    /**
     * @brief Stop service - should shutdown gracefully
     * Requirements:
     * 1. Should be idempotent (can be called multiple times)
     * 2. Should cleanup all resources
     * 3. Should stop all background threads
     */
    virtual void Stop() = 0;

    /**
     * @brief Check if service is currently running
     * @return true if service is running, false otherwise
     */
    virtual bool IsRunning() const = 0;
};

//==============================================================================
// Service Registry Types
//==============================================================================
using ServiceCreator = std::function<std::unique_ptr<ServiceBase>()>;

struct ServiceInfo {
    std::string descriptor;
    ServiceCreator creator;
    void *delegator;
    std::unique_ptr<ServiceBase> instance;
    bool is_running = false;

    ServiceInfo(const std::string &desc, ServiceCreator func, void *del)
        : descriptor(desc), creator(std::move(func)), delegator(del)
    {
    }
};

//==============================================================================
// Service Manager (Singleton)
//==============================================================================
class ServiceManager : public Singleton<ServiceManager> {
    friend class Singleton<ServiceManager>;

public:
    ~ServiceManager() override = default;

    template <typename T>
    bool Register(const std::string &descriptor, ServiceDelegator<T> *delegator);
    void Unregister(const std::string &descriptor);

    std::vector<std::string> GetAllServices() const;
    size_t GetServiceCount() const;
    const ServiceInfo *GetServiceInfo(const std::string &descriptor) const;

    bool StartService(const std::string &descriptor);
    void StopService(const std::string &descriptor);
    bool StartAllServices();
    void StopAllServices();
    bool IsServiceRunning(const std::string &descriptor) const;

protected:
    ServiceManager() = default;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ServiceInfo>> services_;
};

//==============================================================================
// Service Delegator (Auto-Registration Helper)
//==============================================================================
template <typename T>
class ServiceDelegator : public NonCopyable {
public:
    ServiceDelegator();
    ~ServiceDelegator();

private:
    std::mutex regMutex_;
    std::string descriptor_;
};

//==============================================================================
// Template Implementations
//==============================================================================

template <typename T>
bool ServiceManager::Register(const std::string &descriptor, ServiceDelegator<T> *delegator)
{
    static_assert(std::is_base_of_v<ServiceBase, T>, "T must inherit from ServiceBase");

    std::lock_guard<std::mutex> lock(mutex_);

    if (services_.find(descriptor) != services_.end()) {
        return false; // Already registered
    }

    auto creator = []() -> std::unique_ptr<ServiceBase> { return std::make_unique<T>(); };

    services_[descriptor] = std::make_unique<ServiceInfo>(descriptor, creator, delegator);
    LOG_DEBUG("Service registered: %s", descriptor.c_str());
    return true;
}

template <typename T>
ServiceDelegator<T>::ServiceDelegator()
{
    std::lock_guard<std::mutex> lock(regMutex_);
    const std::string descriptor = T::GetName();
    auto registration = ServiceManager::GetInstance();
    if (registration->Register<T>(descriptor, this)) {
        descriptor_ = descriptor;
    }
}

template <typename T>
ServiceDelegator<T>::~ServiceDelegator()
{
    std::lock_guard<std::mutex> lock(regMutex_);
    if (!descriptor_.empty()) {
        auto registration = ServiceManager::GetInstance();
        registration->Unregister(descriptor_);
    }
}

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_SERVICE_MANAGER_H