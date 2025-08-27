/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_X11_SCREEN_CAPTURE_ENGINE_H
#define LMSHAO_REMOTE_DESK_X11_SCREEN_CAPTURE_ENGINE_H

#ifdef __linux__

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include "../iscreen_capture_engine.h"

// X11 forward declarations
#include <X11/Xlib.h>

namespace lmshao::remotedesk {

/**
 * @brief X11 based screen capture engine
 */
class X11ScreenCaptureEngine : public IScreenCaptureEngine {
public:
    X11ScreenCaptureEngine();
    ~X11ScreenCaptureEngine() override;

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
     * @brief Initialize X11 display connection
     * @return CaptureResult indicating success or failure
     */
    CaptureResult InitializeX11();

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
     * @brief Capture frame using XGetImage
     * @return Shared pointer to frame data
     */
    std::shared_ptr<Frame> CaptureFrameXGetImage();

    /**
     * @brief Convert XImage to Frame format
     * @param ximage X11 image to convert
     * @return Shared pointer to frame data
     */
    std::shared_ptr<Frame> ConvertXImageToFrame(XImage *ximage);

    /**
     * @brief Capture cursor if enabled
     * @param frame Frame to overlay cursor on
     */
    void CaptureCursor(std::shared_ptr<Frame> frame);

    /**
     * @brief Cleanup X11 resources
     */
    void Cleanup();

    /**
     * @brief Get screen geometry for specified monitor
     * @param monitor_index Monitor index
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @param width Output width
     * @param height Output height
     * @return true if successful
     */
    bool GetScreenGeometry(uint32_t monitor_index, int &x, int &y, uint32_t &width, uint32_t &height);

private:
    // X11 display and screen information (demo mode - minimal state)
    Display *display_;
    Window root_window_;
    Screen *screen_;
    int screen_number_;

    // Capture thread
    std::unique_ptr<std::thread> capture_thread_;
    std::atomic<bool> should_stop_{false};
    mutable std::mutex mutex_;

    // Frame timing
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::milliseconds frame_interval_;

    // Capture region
    int capture_x_;
    int capture_y_;
    uint32_t capture_width_;
    uint32_t capture_height_;
};

} // namespace lmshao::remotedesk

#endif // __linux__

#endif // LMSHAO_REMOTE_DESK_X11_SCREEN_CAPTURE_ENGINE_H