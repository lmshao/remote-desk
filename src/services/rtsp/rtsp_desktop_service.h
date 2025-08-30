/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_RTSP_DESKTOP_SERVICE_H
#define LMSHAO_REMOTE_DESK_RTSP_DESKTOP_SERVICE_H

#include <network/tcp_server.h>

#include <map>
#include <memory>
#include <mutex>

#include "../../core/pipeline.h"
#include "../../core/service_manager.h"
#include "../../processors/video_encoder.h"
#include "../../sinks/rtp_sender.h"
#include "../../sources/desktop_capture_source.h"

namespace lmshao::remotedesk {

using namespace lmshao::network;

/**
 * @brief RTSP desktop service configuration
 */
struct RTSPDesktopServiceConfig {
    uint16_t rtsp_port = 8554;
    std::string stream_path = "/desktop";

    // Desktop capture configuration
    DesktopCaptureConfig capture_config;

    // Video encoding configuration
    VideoEncoderConfig encoder_config;

    // Service configuration
    bool enable_authentication = false;
    std::string username;
    std::string password;
    uint32_t max_clients = 10;
};

/**
 * @brief RTSP desktop service - main service
 * Inherits from ServiceBase, can be managed by ServiceManager
 * Combines desktop capture source, video encoder and RTP sender
 */
class RTSPDesktopService : public ServiceBase {
public:
    explicit RTSPDesktopService(const RTSPDesktopServiceConfig &config = {});
    ~RTSPDesktopService() override;

    // ServiceBase interface implementation
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;

    /**
     * @brief Get RTSP URL
     */
    std::string GetRTSPUrl() const;

    /**
     * @brief Update configuration
     */
    bool UpdateConfig(const RTSPDesktopServiceConfig &config);

    /**
     * @brief Get current configuration
     */
    const RTSPDesktopServiceConfig &GetConfig() const { return config_; }

    /**
     * @brief Get service statistics
     */
    struct ServiceStats {
        uint32_t active_clients = 0;
        uint64_t total_connections = 0;
        uint64_t frames_captured = 0;
        uint64_t frames_encoded = 0;
        uint64_t frames_sent = 0;
        double current_fps = 0.0;
        double current_bitrate = 0.0;
    };
    ServiceStats GetStats() const;

    /**
     * @brief Force keyframe (all clients)
     */
    void ForceKeyFrame();

    // Service registration macro - used for ServiceManager auto registration
    REGISTER_SERVICE(RTSPDesktopService, "RTSPDesktopService")

private:
    /**
     * @brief RTSP server listener implementation
     */
    class RTSPServerListener {
    public:
        explicit RTSPServerListener(RTSPDesktopService *service);
        virtual ~RTSPServerListener() = default;

        virtual void OnClientConnected(const std::string &client_ip, const std::string &user_agent);
        virtual void OnClientDisconnected(const std::string &client_ip);
        virtual void OnStreamRequested(const std::string &stream_path);
        virtual void OnSetupReceived(const std::string &client_ip, const std::string &transport);
        virtual void OnPlayReceived(const std::string &client_ip);
        virtual void OnPauseReceived(const std::string &client_ip);
        virtual void OnTeardownReceived(const std::string &client_ip);

    private:
        RTSPDesktopService *service_;
    };

    /**
     * @brief Client session information
     */
    struct ClientSession {
        std::string client_ip;
        std::string user_agent;
        std::shared_ptr<RTPSender> rtp_sender;
        std::shared_ptr<Pipeline> pipeline;
        std::chrono::steady_clock::time_point connect_time;
        uint64_t frames_sent = 0;
    };

    /**
     * @brief Initialize RTSP server
     */
    bool InitializeRTSPServer();

    /**
     * @brief Create client session
     */
    bool CreateClientSession(const std::string &client_ip, const std::string &transport);

    /**
     * @brief Remove client session
     */
    void RemoveClientSession(const std::string &client_ip);

    /**
     * @brief Get or create shared desktop capture source
     */
    std::shared_ptr<DesktopCaptureSource> GetSharedCaptureSource();

    /**
     * @brief Get or create shared video encoder
     */
    std::shared_ptr<VideoEncoder> GetSharedVideoEncoder();

private:
    RTSPDesktopServiceConfig config_;
    std::atomic<bool> running_{false};

    // RTSP server
    std::shared_ptr<TcpServer> rtsp_server_;
    std::shared_ptr<RTSPServerListener> server_listener_;

    // Shared components (multi-client sharing)
    std::shared_ptr<DesktopCaptureSource> shared_capture_source_;
    std::shared_ptr<VideoEncoder> shared_video_encoder_;

    // Client session management
    std::mutex clients_mutex_;
    std::map<std::string, std::unique_ptr<ClientSession>> client_sessions_;

    // Statistics
    mutable std::mutex stats_mutex_;
    ServiceStats stats_;
    std::chrono::steady_clock::time_point service_start_time_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_RTSP_DESKTOP_SERVICE_H