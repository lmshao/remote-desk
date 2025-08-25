/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_DISCOVERY_SERVICE_H
#define LMSHAO_REMOTE_DESK_DISCOVERY_SERVICE_H

#include <network/iserver_listener.h>
#include <network/udp_client.h>
#include <network/udp_server.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace lmshao::remotedesk {

using namespace lmshao::network;

class InternalServerListener;
struct DiscoveryInfo {
    std::string type;
    std::string id;
    std::string ip;
    uint16_t port;
    std::string version;
};

class DiscoveryListener {
public:
    virtual ~DiscoveryListener() = default;
    virtual void OnFound(const DiscoveryInfo &info) = 0;
};

class DiscoveryService {
    friend class InternalServerListener;

public:
    DiscoveryService(const std::string &type, uint16_t port, const std::string &version = "1.0.0");
    ~DiscoveryService();
    void Start();
    void Stop();
    void SetListener(std::shared_ptr<DiscoveryListener> listener);

private:
    void DiscoveryLoop();
    std::string type_;
    std::string id_;
    std::string version_;
    uint16_t port_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> discovery_thread_;
    std::weak_ptr<DiscoveryListener> listener_;
    std::unique_ptr<UdpServer> server_;
    std::unique_ptr<UdpClient> client_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_DISCOVERY_SERVICE_H
