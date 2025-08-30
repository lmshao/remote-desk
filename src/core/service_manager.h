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
#include <coreutils/task_queue.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../log/remote_desk_log.h"
#include "service_message.h"

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
    virtual ~ServiceBase() { StopTaskQueue(); }

    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;

    void SetServiceNotifier(const ServiceEventHandler &notifier) { notifier_ = notifier; }

protected:
    /**
     * @brief Notify main service about an event (async)
     */
    void NotifyMainService(const ServiceMessage &message)
    {
        if (!notifier_ || !IsRunning()) {
            return;
        }

        EnsureTaskQueueStarted();
        auto task = std::make_shared<TaskHandler<void>>([notifier = notifier_, message]() { notifier(message); });
        task_queue_->EnqueueTask(task);
    }

    /**
     * @brief Enqueue async task for business logic
     */
    template <typename Func>
    void EnqueueTask(Func &&func, uint64_t delayUs = 0)
    {
        if (!IsRunning()) {
            return;
        }

        EnsureTaskQueueStarted();
        auto task = std::make_shared<TaskHandler<void>>(std::forward<Func>(func));
        task_queue_->EnqueueTask(task, false, delayUs);
    }

    /**
     * @brief Get service name for queue identification
     * Override this method to provide meaningful service name
     */
    virtual std::string GetServiceName() const
    {
        return "Service_" + std::to_string(reinterpret_cast<uintptr_t>(this));
    }

private:
    /**
     * @brief Lazy initialization of task queue
     */
    void EnsureTaskQueueStarted()
    {
        if (!task_queue_) {
            task_queue_ = std::make_unique<TaskQueue>(GetServiceName() + "_Queue");
            task_queue_->Start();
        }
    }

    /**
     * @brief Stop and cleanup task queue
     */
    void StopTaskQueue()
    {
        if (task_queue_) {
            task_queue_->Stop();
            task_queue_.reset();
        }
    }

    ServiceEventHandler notifier_;
    std::unique_ptr<TaskQueue> task_queue_;
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
    ~ServiceManager();

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

    // Service event callback management
    void SetEventCallback(const ServiceEventHandler &callback);

protected:
    ServiceManager();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<ServiceInfo>> services_;
    ServiceEventHandler event_callback_;
    std::mutex callback_mutex_;

    // Internal notification method
    void NotifyMainService(const ServiceMessage &message);
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

    auto creator = [this]() -> std::unique_ptr<ServiceBase> {
        auto service = std::make_unique<T>();
        service->SetServiceNotifier([this](const ServiceMessage &msg) { this->NotifyMainService(msg); });
        return service;
    };

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