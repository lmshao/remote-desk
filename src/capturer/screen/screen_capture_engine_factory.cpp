/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "screen_capture_engine_factory.h"

#ifdef _WIN32
#include "desktop_duplication/desktop_duplication_screen_capture_engine.h"
#endif

#ifdef __linux__
#include "x11/x11_screen_capture_engine.h"
// #include "pipewire/pipewire_screen_capture_engine.h" // Future implementation
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

        case Technology::PipeWire:
            return CreatePipeWireEngine();

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
    // Prefer X11 for now, PipeWire support can be added later
    return Technology::X11;
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

        case Technology::PipeWire:
            // Future implementation for Linux
            return false;

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

        case Technology::PipeWire:
            return "PipeWire API (Linux)";

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

std::unique_ptr<IScreenCaptureEngine> ScreenCaptureEngineFactory::CreatePipeWireEngine()
{
#ifdef __linux__
    // Future implementation for PipeWire
    // try {
    //     return std::make_unique<PipeWireScreenCaptureEngine>();
    // } catch (const std::exception&) {
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