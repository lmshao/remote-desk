/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_DESKTOP_DUPLICATION_SCREEN_CAPTURE_ENGINE_H
#define LMSHAO_REMOTE_DESK_DESKTOP_DUPLICATION_SCREEN_CAPTURE_ENGINE_H

#ifdef _WIN32

#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <wrl/client.h>

#include <atomic>
#include <mutex>
#include <thread>

#include "../iscreen_capture_engine.h"

namespace lmshao::remotedesk {

/**
 * @brief Desktop Duplication API based screen capture engine
 */
class DesktopDuplicationScreenCaptureEngine : public IScreenCaptureEngine {
public:
    DesktopDuplicationScreenCaptureEngine();
    ~DesktopDuplicationScreenCaptureEngine() override;

    // IScreenCaptureEngine implementation
    CaptureResult Initialize(const ScreenCaptureConfig &config) override;
    CaptureResult Start() override;
    void Stop() override;
    bool IsRunning() const override;
    std::vector<ScreenInfo> GetAvailableScreens() const override;
    void SetFrameCallback(std::function<void(std::shared_ptr<Frame>)> callback) override;
    const ScreenCaptureConfig &GetConfig() const override;
    CaptureResult UpdateConfig(const ScreenCaptureConfig &config) override;

private:
    /**
     * @brief Initialize D3D11 device and context
     * @return CaptureResult indicating success or failure
     */
    CaptureResult InitializeD3D();

    /**
     * @brief Initialize Desktop Duplication API
     * @return CaptureResult indicating success or failure
     */
    CaptureResult InitializeDesktopDuplication();

    /**
     * @brief Capture thread main function
     */
    void CaptureThreadProc();

    /**
     * @brief Capture a single frame
     * @return CaptureResult indicating success or failure
     */
    CaptureResult CaptureFrame();

    /**
     * @brief Convert DXGI surface to frame data
     * @param surface DXGI surface to convert
     * @return Shared pointer to frame data
     */
    std::shared_ptr<Frame> ConvertSurfaceToFrame(Microsoft::WRL::ComPtr<IDXGISurface> surface);

    /**
     * @brief Cleanup resources
     */
    void Cleanup();

    /**
     * @brief Set last error message
     * @param error Error message
     */
    void LOG_ERROR(const std::string &error);

private:
    // D3D11 resources
    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context_;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> desktop_duplication_;
    Microsoft::WRL::ComPtr<IDXGIOutput1> dxgi_output_;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgi_adapter_;

    // Capture thread
    std::unique_ptr<std::thread> capture_thread_;
    std::atomic<bool> should_stop_{false};
    mutable std::mutex mutex_;

    // Frame timing
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::milliseconds frame_interval_;
};

} // namespace lmshao::remotedesk

#endif // _WIN32

#endif // LMSHAO_REMOTE_DESK_DESKTOP_DUPLICATION_SCREEN_CAPTURE_ENGINE_H