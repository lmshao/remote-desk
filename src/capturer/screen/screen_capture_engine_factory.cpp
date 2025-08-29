/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "screen_capture_engine_factory.h"

#include <cstdlib>

#include "../log/remote_desk_log.h"

#ifdef _WIN32
#include "desktop_duplication/desktop_duplication_screen_capture_engine.h"
#endif

#ifdef __linux__
#include "x11/x11_screen_capture_engine.h"
// #include "wayland/wayland_tool_engine.h"  // Not implemented yet
#endif

#ifdef __APPLE__
// #include "core_graphics/core_graphics_screen_capture_engine.h" // Future implementation
#endif

namespace lmshao::remotedesk {

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreateEngine(Technology technology)
{
    Technology target_technology = technology;

    // Auto-detect best available technology if requested
    if (technology == Technology::Auto) {
        target_technology = GetBestAvailableTechnology();
    }

    // Check if the technology is supported
    if (!IsTechnologySupported(target_technology)) {
        return nullptr;
    }

    // Create technology-specific engine
    switch (target_technology) {
        case Technology::DesktopDuplication:
            return CreateDesktopDuplicationEngine();

        case Technology::X11:
            return CreateX11Engine();

        case Technology::WaylandTool:
            return CreateWaylandToolEngine();

        case Technology::CoreGraphics:
            return CreateCoreGraphicsEngine();

        default:
            return nullptr;
    }
}

ScreenCaptureEngineFactory::Technology ScreenCaptureEngineFactory::GetBestAvailableTechnology()
{
#ifdef _WIN32
    return Technology::DesktopDuplication;
#elif defined(__linux__)
    // Environment detection logic
    const char *wayland_display = std::getenv("WAYLAND_DISPLAY");
    const char *x11_display = std::getenv("DISPLAY");
    // bool wayland_tool_available = WaylandToolEngine::IsAvailable();  // Not implemented yet

    // Priority: X11 for now
    if (x11_display) {
        return Technology::X11;
    } else if (wayland_display) {
        // Wayland without proper tools - warn user
        LOG_WARN("Wayland detected but Wayland tools not available");
        LOG_WARN("Screen capture may not work. Consider using X11 mode via XWayland");
        return Technology::X11; // Fallback to X11 even in Wayland (via XWayland)
    } else {
        // Final fallback to X11
        return Technology::X11;
    }
#elif defined(__APPLE__)
    return Technology::CoreGraphics;
#else
    // Unknown platform
    return Technology::Auto; // This will cause creation to fail
#endif
}

bool ScreenCaptureEngineFactory::IsTechnologySupported(Technology technology)
{
    switch (technology) {
        case Technology::DesktopDuplication:
#ifdef _WIN32
            return true;
#else
            return false;
#endif

        case Technology::X11:
#ifdef __linux__
            return true;
#else
            return false;
#endif

        case Technology::WaylandTool:
#ifdef __linux__
            // return WaylandToolEngine::IsAvailable();  // Not implemented yet
            return false;
#else
            return false;
#endif

        case Technology::CoreGraphics:
#ifdef __APPLE__
            // return true; // Enable when macOS implementation is ready
            return false; // Currently not implemented
#else
            return false;
#endif

        case Technology::Auto:
            return IsTechnologySupported(GetBestAvailableTechnology());

        default:
            return false;
    }
}

std::string ScreenCaptureEngineFactory::GetTechnologyName(Technology technology)
{
    switch (technology) {
        case Technology::DesktopDuplication:
            return "Desktop Duplication API (Windows)";

        case Technology::X11:
            return "X11 API (Linux/Unix)";

        case Technology::WaylandTool:
            return "Wayland Tools (Linux)";

        case Technology::CoreGraphics:
            return "Core Graphics (macOS)";

        case Technology::Auto:
            return "Auto-detect (" + GetTechnologyName(GetBestAvailableTechnology()) + ")";

        default:
            return "Unknown Technology";
    }
}

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreateDesktopDuplicationEngine()
{
#ifdef _WIN32
    try {
        return std::make_unique<DesktopDuplicationScreenCaptureEngine>();
    } catch (const std::exception &) {
        return nullptr;
    }
#else
    return nullptr;
#endif
}

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreateX11Engine()
{
#ifdef __linux__
    try {
        return std::make_unique<X11ScreenCaptureEngine>();
    } catch (const std::exception &) {
        return nullptr;
    }
#else
    return nullptr;
#endif
}

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreateWaylandToolEngine()
{
#ifdef __linux__
    // try {
    //     return std::make_unique<WaylandToolEngine>();
    // } catch (const std::exception &) {
    //     return nullptr;
    // }
    return nullptr; // Not implemented yet
#else
    return nullptr;
#endif
}

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreateCoreGraphicsEngine()
{
#ifdef __APPLE__
    // Future implementation for macOS
    // try {
    //     return std::make_unique<CoreGraphicsScreenCaptureEngine>();
    // } catch (const std::exception&) {
    //     return nullptr;
    // }
    return nullptr; // Not implemented yet
#else
    return nullptr;
#endif
}

} // namespace lmshao::remotedesk