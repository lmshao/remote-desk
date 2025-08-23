
/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "discovery_service.h"

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

#include "remote_desk_log.h"

using namespace lmshao::network;

namespace lmshao::remotedesk {

namespace {
constexpr const char *BROADCAST_ADDR = "255.255.255.255";
constexpr uint16_t BROADCAST_PORT = 19000;
constexpr int BROADCAST_INTERVAL_MS = 1000;
} // namespace

DiscoveryService::DiscoveryService(const std::string &type, uint16_t port, const std::string &version)
    : type_(type), id_(), version_(version), port_(port), running_(false)
{
    std::random_device rd;
    id_ = std::to_string(rd());
}

DiscoveryService::~DiscoveryService()
{
    Stop();
}

void DiscoveryService::Start()
{
    running_ = true;
    discovery_thread_ = std::make_unique<std::thread>(&DiscoveryService::DiscoveryLoop, this);
}

void DiscoveryService::Stop()
{
    running_ = false;
    if (discovery_thread_ && discovery_thread_->joinable())
        discovery_thread_->join();
}

void DiscoveryService::SetListener(std::shared_ptr<DiscoveryListener> listener)
{
    listener_ = listener;
}

class InternalServerListener : public IServerListener {
public:
    InternalServerListener(DiscoveryService *svc) : svc_(svc) {}

    void OnAccept(std::shared_ptr<Session>) override {}

    void OnReceive(std::shared_ptr<Session> session, std::shared_ptr<lmshao::coreutils::DataBuffer> buffer) override
    {
        try {
            std::string data((const char *)buffer->Data(), buffer->Size());
            size_t p1 = data.find('|');
            size_t p2 = data.find('|', p1 + 1);
            size_t p3 = data.find('|', p2 + 1);
            if (p1 != std::string::npos && p2 != std::string::npos && p3 != std::string::npos) {
                DiscoveryInfo info;
                info.type = data.substr(0, p1);
                info.id = data.substr(p1 + 1, p2 - p1 - 1);
                info.port = static_cast<uint16_t>(std::stoi(data.substr(p2 + 1, p3 - p2 - 1)));
                info.version = data.substr(p3 + 1);
                info.ip = session->ClientInfo();

                // Only notify if it's a different service of the same type
                if (info.id != svc_->id_ && info.type == svc_->type_) {
                    if (auto listener = svc_->listener_.lock()) {
                        listener->OnFound(info);
                    }
                }
            }
        } catch (...) {
            // Ignore malformed messages
            LOG_WARN("Malformed discovery message received: %s", "parsing error");
        }
    }

    void OnClose(std::shared_ptr<Session>) override {}
    void OnError(std::shared_ptr<Session>, const std::string &) override {}

private:
    DiscoveryService *svc_;
};

void DiscoveryService::DiscoveryLoop()
{
    // Create UDP server for receiving
    server_ = std::make_unique<UdpServer>(BROADCAST_PORT);
    auto serverListener = std::make_shared<InternalServerListener>(this);
    server_->SetListener(serverListener);

    if (!server_->Init()) {
        LOG_ERROR("Failed to initialize UDP server for discovery");
        return;
    }

    if (!server_->Start()) {
        LOG_ERROR("Failed to start UDP server for discovery");
        return;
    }

    // Create UDP client for sending broadcasts
    client_ = std::make_unique<UdpClient>(BROADCAST_ADDR, BROADCAST_PORT);

    if (!client_->Init()) {
        LOG_ERROR("Failed to initialize UDP client for broadcast");
        server_->Stop();
        return;
    }

    // Enable broadcast functionality
    if (!client_->EnableBroadcast()) {
        LOG_ERROR("Failed to enable broadcast on UDP client");
        server_->Stop();
        client_->Close();
        return;
    }

    auto last_broadcast = std::chrono::steady_clock::now();

    while (running_) {
        auto now = std::chrono::steady_clock::now();

        // Send broadcast message every interval
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_broadcast).count() >=
            BROADCAST_INTERVAL_MS) {
            // Format: type|id|port|version
            std::ostringstream oss;
            oss << type_ << '|' << id_ << '|' << port_ << '|' << version_;
            std::string msg = oss.str();
            client_->Send(msg);
            last_broadcast = now;
        }

        // Sleep briefly to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    server_->Stop();
    client_->Close();
}

} // namespace lmshao::remotedesk
