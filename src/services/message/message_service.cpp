/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "message_service.h"

#include <thread>

#include "../../log/remote_desk_log.h"

namespace lmshao::remotedesk {

MessageService::MessageService()
{
    LOG_DEBUG("MessageService created");
}

MessageService::~MessageService()
{
    Stop();
    LOG_DEBUG("MessageService destroyed");
}

bool MessageService::Start()
{
    if (running_.load()) {
        return true;
    }

    LOG_DEBUG("Starting MessageService on port %d", port_);

    // Initialize resources (non-blocking)
    // Example: create socket, bind port, etc.

    // Start background worker thread if needed
    // worker_thread_ = std::thread(&MessageService::WorkerLoop, this);

    // Set running state
    running_.store(true);

    LOG_DEBUG("MessageService started successfully on port %d", port_);
    return true;
}

void MessageService::Stop()
{
    if (!running_.load()) {
        return;
    }

    LOG_DEBUG("Stopping MessageService");

    // TODO: Cleanup HTTP server here
    // For now, just simulate shutdown
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    running_.store(false);
    LOG_DEBUG("MessageService stopped");
}

bool MessageService::IsRunning() const
{
    return running_.load();
}

} // namespace lmshao::remotedesk
