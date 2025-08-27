/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_ISCREEN_CAPTURE_ENGINE_H
#define LMSHAO_REMOTE_DESK_ISCREEN_CAPTURE_ENGINE_H

#include <functional>
#include <memory>
#include <vector>

#include "../../core/frame.h"
#include "screen_capture_config.h"

namespace lmshao::remotedesk {

/**
 * @brief Abstract interface for platform-specific screen capture engines
 */
class IScreenCaptureEngine {
public:
    virtual ~IScreenCaptureEngine() = default;

    /**
     * @brief Initialize the capture engine with given configuration
     * @param config Screen capture configuration
     * @return CaptureResult indicating success or failure
     */
    virtual CaptureResult Initialize(const ScreenCaptureConfig &config) = 0;

    /**
     * @brief Start screen capture
     * @return CaptureResult indicating success or failure
     */
    virtual CaptureResult Start() = 0;

    /**
     * @brief Stop screen capture
     */
    virtual void Stop() = 0;

    /**
     * @brief Check if capture engine is currently running
     * @return true if running, false otherwise
     */
    virtual bool IsRunning() const = 0;

    /**
     * @brief Get available screens/monitors information
     * @return Vector of screen information structures
     */
    virtual std::vector<ScreenInfo> GetAvailableScreens() const = 0;

    /**
     * @brief Set frame callback function
     * @param callback Function to be called when a new frame is captured
     */
    virtual void SetFrameCallback(std::function<void(std::shared_ptr<Frame>)> callback) = 0;

    /**
     * @brief Get current capture configuration
     * @return Current screen capture configuration
     */
    virtual const ScreenCaptureConfig &GetConfig() const = 0;

    /**
     * @brief Update capture configuration (may require restart)
     * @param config New configuration
     * @return CaptureResult indicating success or failure
     */
    virtual CaptureResult UpdateConfig(const ScreenCaptureConfig &config) = 0;

protected:
    /**
     * @brief Current capture configuration
     */
    ScreenCaptureConfig config_;

    /**
     * @brief Frame callback function
     */
    std::function<void(std::shared_ptr<Frame>)> frame_callback_;

    /**
     * @brief Last error message
     */
    std::string last_error_;

    /**
     * @brief Running state flag
     */
    bool is_running_ = false;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_ISCREEN_CAPTURE_ENGINE_H