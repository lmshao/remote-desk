/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <memory>

#include "../../core/media_source.h"
#include "iscreen_capture_engine.h"
#include "screen_capture_config.h"
#include "screen_capture_engine_factory.h"

namespace lmshao::remotedesk {

/**
 * @brief Screen capturer implementation using cross-platform architecture
 *
 * This class provides screen capture functionality as a media source.
 * It uses a platform-specific capture engine internally to handle the actual
 * screen capture operations while providing a unified interface.
 */
class ScreenCapturer : public MediaSource {
public:
    /**
     * @brief Constructor with configuration
     *
     * @param config Screen capture configuration
     * @param technology The target capture technology for screen capture (auto-detect if not specified)
     */
    explicit ScreenCapturer(const ScreenCaptureConfig &config, ScreenCaptureEngineFactory::Technology technology =
                                                                   ScreenCaptureEngineFactory::Technology::Auto);

    ~ScreenCapturer() override;

    // MediaSource interface
    bool Initialize() override;
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;

    /**
     * @brief Get available screens/monitors
     *
     * @return std::vector<ScreenInfo> List of available screens
     */
    std::vector<ScreenInfo> GetAvailableScreens() const;

    /**
     * @brief Get current configuration
     *
     * @return const ScreenCaptureConfig& Current configuration
     */
    const ScreenCaptureConfig &GetConfig() const;

    /**
     * @brief Update configuration
     *
     * @param config New configuration
     * @return CaptureResult Result of configuration update
     */
    CaptureResult UpdateConfig(const ScreenCaptureConfig &config);

    /**
     * @brief Get the technology name being used
     *
     * @return std::string Technology name
     */
    std::string GetTechnologyName() const;

private:
    /**
     * @brief Frame callback handler
     *
     * @param frame Captured frame
     */
    void OnFrameCaptured(std::shared_ptr<Frame> frame);

private:
    std::unique_ptr<IScreenCaptureEngine> engine_;      ///< Platform-specific capture engine
    ScreenCaptureConfig config_;                        ///< Current configuration
    ScreenCaptureEngineFactory::Technology technology_; ///< Target technology
    bool initialized_;                                  ///< Initialization state
};

} // namespace lmshao::remotedesk