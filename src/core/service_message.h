/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_SERVICE_MESSAGE_H
#define LMSHAO_REMOTE_DESK_SERVICE_MESSAGE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace lmshao::remotedesk {

enum ServiceType : uint8_t {
    MAIN_SERVICE = 0,
    RTSP_SERVICE,
};

enum EventType : uint8_t {
    EVENT_UNKNOWN = 0,
    EVENT_CONNECT,
    EVENT_DISCONNECT,
    EVENT_STREAM_REQUEST,
};

struct ServiceMessage {
    ServiceType sender;
    EventType event;
    std::string data;
};

/**
 * @brief Function type for service event communication
 * Used for both receiving events and notifying main service
 */
using ServiceEventHandler = std::function<void(const ServiceMessage &message)>;

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_SERVICE_MESSAGE_H