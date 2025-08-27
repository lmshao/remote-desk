/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>

#include "iscreen_capture_engine.h"

namespace lmshao::remotedesk {

/**
 * @brief Factory class for creating technology-specific screen capture engines
 *
 * This factory provides a unified interface for creating screen capture engines
 * based on different capture technologies. It abstracts away the technology-specific
 * implementation details and provides a clean way to instantiate the correct engine.
 */
class ScreenCaptureEngineFactory {
public:
    /**
     * @brief Capture technologies supported by the factory
     */
    enum class Technology {
        DesktopDuplication, ///< Windows Desktop Duplication API
        X11,                ///< X11 API (Linux/Unix)
        PipeWire,           ///< PipeWire API (Linux, future support)
        CoreGraphics,       ///< macOS Core Graphics (future support)
        Auto                ///< Automatically detect best available technology
    };

    /**
     * @brief Create a screen capture engine for the specified technology
     *
     * @param technology The target capture technology for the engine
     * @return std::unique_ptr<IScreenCaptureEngine> Pointer to the created engine, or nullptr on failure
     */
    static std::unique_ptr<IScreenCaptureEngine> CreateEngine(Technology technology = Technology::Auto);

    /**
     * @brief Get the best available technology for current platform
     *
     * @return Technology The best available capture technology
     */
    static Technology GetBestAvailableTechnology();

    /**
     * @brief Check if a platform is supported
     *
     * @param platform The platform to check
     * @return bool True if the platform is supported, false otherwise
     */
    static bool IsTechnologySupported(Technology technology);

    /**
     * @brief Get a human-readable name for the platform
     *
     * @param platform The platform to get the name for
     * @return std::string The platform name
     */
    static std::string GetTechnologyName(Technology technology);

private:
    /**
     * @brief Create Desktop Duplication screen capture engine
     *
     * @return std::unique_ptr<IScreenCaptureEngine> Pointer to Desktop Duplication engine, or nullptr on failure
     */
    static std::unique_ptr<IScreenCaptureEngine> CreateDesktopDuplicationEngine();

    /**
     * @brief Create X11 screen capture engine
     *
     * @return std::unique_ptr<IScreenCaptureEngine> Pointer to X11 engine, or nullptr on failure
     */
    static std::unique_ptr<IScreenCaptureEngine> CreateX11Engine();

    /**
     * @brief Create PipeWire screen capture engine
     *
     * @return std::unique_ptr<IScreenCaptureEngine> Pointer to PipeWire engine, or nullptr on failure
     */
    static std::unique_ptr<IScreenCaptureEngine> CreatePipeWireEngine();

    /**
     * @brief Create Core Graphics screen capture engine
     *
     * @return std::unique_ptr<IScreenCaptureEngine> Pointer to Core Graphics engine, or nullptr on failure
     */
    static std::unique_ptr<IScreenCaptureEngine> CreateCoreGraphicsEngine();
};

} // namespace lmshao::remotedesk