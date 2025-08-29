/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_SCREEN_CAPTURE_CONFIG_H
#define LMSHAO_REMOTE_DESK_SCREEN_CAPTURE_CONFIG_H

#include <cstdint>
#include <string>

namespace lmshao::remotedesk {

/**
 * @brief Screen capture configuration structure
 */
struct ScreenCaptureConfig {
    /**
     * @brief Target frame rate for screen capture
     */
    uint32_t frame_rate = 30;

    /**
     * @brief Capture region width (0 = full screen width)
     */
    uint32_t width = 0;

    /**
     * @brief Capture region height (0 = full screen height)
     */
    uint32_t height = 0;

    /**
     * @brief X offset of capture region
     */
    int32_t offset_x = 0;

    /**
     * @brief Y offset of capture region
     */
    int32_t offset_y = 0;

    /**
     * @brief Monitor index to capture (0 = primary monitor)
     */
    uint32_t monitor_index = 0;

    /**
     * @brief Enable cursor capture
     */
    bool capture_cursor = true;

    /**
     * @brief Enable hardware acceleration if available
     */
    bool use_hardware_acceleration = true;

    /**
     * @brief Pixel format preference (platform-specific)
     */
    std::string pixel_format = "BGRA";
};

/**
 * @brief Screen capture result codes
 */
enum class CaptureResult {
    Success = 0,
    ErrorInitialization,
    ErrorInvalidConfig,
    ErrorNoDisplay,
    ErrorAccessDenied,
    ErrorTimeout,
    ErrorUnknown,
    NotSupported,
    AlreadyStarted,
    AlreadyInitialized
};

/**
 * @brief Screen information structure
 */
struct ScreenInfo {
    uint32_t id = 0;         ///< Screen identifier
    uint32_t width;          ///< Screen width in pixels
    uint32_t height;         ///< Screen height in pixels
    uint32_t bits_per_pixel; ///< Bits per pixel
    int32_t x = 0;           ///< X coordinate of screen
    int32_t y = 0;           ///< Y coordinate of screen
    std::string name;        ///< Display/monitor name
    bool is_primary;         ///< Whether this is the primary display
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_SCREEN_CAPTURE_CONFIG_H