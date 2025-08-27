/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef __linux__

#include "x11_screen_capture_engine.h"

#include <chrono>
#include <cstring>
#include <thread>

#include "../../../log/remote_desk_log.h"

// X11 headers for real screen capture
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Undefine X11 macros that conflict with our enums
#ifdef Success
#undef Success
#endif

namespace lmshao::remotedesk {

X11ScreenCaptureEngine::X11ScreenCaptureEngine()
    : display_(nullptr), root_window_(0), screen_(nullptr), screen_number_(0), capture_thread_(nullptr),
      should_stop_(false), last_frame_time_(), frame_interval_(std::chrono::milliseconds(33)), // Default 30 FPS
      capture_x_(0), capture_y_(0), capture_width_(0), capture_height_(0)
{
    LOG_DEBUG("X11ScreenCaptureEngine created");
}

X11ScreenCaptureEngine::~X11ScreenCaptureEngine()
{
    Stop();
    Cleanup();
    LOG_DEBUG("X11ScreenCaptureEngine destroyed");
}

CaptureResult X11ScreenCaptureEngine::Initialize(const ScreenCaptureConfig &config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_running_) {
        LOG_ERROR("Cannot initialize while capture is running");
        return CaptureResult::ErrorInitialization;
    }

    LOG_DEBUG("Initializing X11 screen capture engine");

    config_ = config;
    frame_interval_ = std::chrono::milliseconds(1000 / config_.frame_rate);

    // Initialize X11 connection
    auto result = InitializeX11();
    if (result != CaptureResult::Success) {
        return result;
    }

    LOG_DEBUG("X11 screen capture engine initialized successfully");
    return CaptureResult::Success;
}

CaptureResult X11ScreenCaptureEngine::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_running_) {
        return CaptureResult::Success;
    }

    LOG_DEBUG("Starting X11 screen capture");

    should_stop_ = false;
    capture_thread_ = std::make_unique<std::thread>(&X11ScreenCaptureEngine::CaptureThreadProc, this);
    is_running_ = true;

    LOG_DEBUG("Linux screen capture started");
    return CaptureResult::Success;
}

void X11ScreenCaptureEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_running_) {
        return;
    }

    LOG_DEBUG("Stopping Linux screen capture");

    should_stop_ = true;
    if (capture_thread_ && capture_thread_->joinable()) {
        capture_thread_->join();
    }
    capture_thread_.reset();
    is_running_ = false;

    LOG_DEBUG("Linux screen capture stopped");
}

bool X11ScreenCaptureEngine::IsRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return is_running_;
}

std::vector<ScreenInfo> X11ScreenCaptureEngine::GetAvailableScreens() const
{
    std::vector<ScreenInfo> screens;

    if (!display_) {
        // If no display connection, return empty list
        return screens;
    }

    // Get screen count
    int screen_count = ScreenCount(display_);

    for (int i = 0; i < screen_count; ++i) {
        Screen *screen = ScreenOfDisplay(display_, i);
        if (screen) {
            ScreenInfo screen_info;
            screen_info.name = "Screen " + std::to_string(i);
            screen_info.width = WidthOfScreen(screen);
            screen_info.height = HeightOfScreen(screen);
            screen_info.bits_per_pixel = DefaultDepthOfScreen(screen);
            screen_info.is_primary = (i == DefaultScreen(display_));

            screens.push_back(screen_info);
        }
    }

    LOG_DEBUG("Available screens: %zu", screens.size());
    return screens;
}

void X11ScreenCaptureEngine::SetFrameCallback(std::function<void(std::shared_ptr<Frame>)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    frame_callback_ = callback;
}

const ScreenCaptureConfig &X11ScreenCaptureEngine::GetConfig() const
{
    return config_;
}

CaptureResult X11ScreenCaptureEngine::UpdateConfig(const ScreenCaptureConfig &config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    bool was_running = is_running_;
    if (was_running) {
        Stop();
    }

    auto result = Initialize(config);
    if (result == CaptureResult::Success && was_running) {
        result = Start();
    }

    return result;
}

CaptureResult X11ScreenCaptureEngine::InitializeX11()
{
    LOG_DEBUG("Initializing X11 display connection");

    // Try to open X11 display connection
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        LOG_ERROR("No X11 display available, this might be a headless environment");
        LOG_ERROR("Consider using Xvfb (virtual framebuffer) for headless screen capture");
        LOG_ERROR("Example: Xvfb :99 -screen 0 1024x768x24 &");
        LOG_ERROR("Then set DISPLAY=:99 before running this program");
        return CaptureResult::ErrorNoDisplay;
    }

    // Get default screen
    screen_number_ = DefaultScreen(display_);
    screen_ = ScreenOfDisplay(display_, screen_number_);
    root_window_ = RootWindow(display_, screen_number_);

    // Set capture area based on config or use full screen
    if (config_.width > 0 && config_.height > 0) {
        capture_width_ = config_.width;
        capture_height_ = config_.height;
        capture_x_ = config_.offset_x;
        capture_y_ = config_.offset_y;
    } else {
        capture_width_ = WidthOfScreen(screen_);
        capture_height_ = HeightOfScreen(screen_);
        capture_x_ = 0;
        capture_y_ = 0;
    }

    LOG_DEBUG("X11 display initialized: %dx%d at (%d,%d)", capture_width_, capture_height_, capture_x_, capture_y_);
    return CaptureResult::Success;
}

void X11ScreenCaptureEngine::CaptureThreadProc()
{
    LOG_DEBUG("X11 capture loop started");

    last_frame_time_ = std::chrono::steady_clock::now();

    while (!should_stop_) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - last_frame_time_;

        if (elapsed >= frame_interval_) {
            CaptureFrame();
            last_frame_time_ = current_time;
        }

        // Small sleep to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LOG_DEBUG("X11 capture loop ended");
}

CaptureResult X11ScreenCaptureEngine::CaptureFrame()
{
    if (!frame_callback_ || !display_) {
        return CaptureResult::Success;
    }

    std::shared_ptr<Frame> frame;

    // In demo mode, only use XGetImage simulation
    frame = CaptureFrameXGetImage();

    if (!frame) {
        return CaptureResult::ErrorUnknown;
    }

    // Set timestamp
    frame->timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();

    // Deliver the frame
    frame_callback_(frame);

    return CaptureResult::Success;
}

std::shared_ptr<Frame> X11ScreenCaptureEngine::CaptureFrameXGetImage()
{
    if (!display_) {
        LOG_ERROR("Display is null in CaptureFrameXGetImage");
        return nullptr;
    }

    LOG_DEBUG("Attempting to capture screen: %dx%d at (%d,%d)", capture_width_, capture_height_, capture_x_,
              capture_y_);

    // Validate capture parameters
    if (capture_width_ == 0 || capture_height_ == 0) {
        LOG_ERROR("Invalid capture dimensions: %dx%d", capture_width_, capture_height_);
        return nullptr;
    }

    // Capture screen using XGetImage
    XImage *ximage =
        XGetImage(display_, root_window_, capture_x_, capture_y_, capture_width_, capture_height_, AllPlanes, ZPixmap);

    if (!ximage) {
        LOG_ERROR("Failed to capture screen with XGetImage: %dx%d at (%d,%d)", capture_width_, capture_height_,
                  capture_x_, capture_y_);
        return nullptr;
    }

    LOG_DEBUG("Successfully captured XImage: %dx%d, depth=%d, bits_per_pixel=%d", ximage->width, ximage->height,
              ximage->depth, ximage->bits_per_pixel);

    // Convert XImage to Frame
    auto frame = ConvertXImageToFrame(ximage);

    // Clean up XImage
    XDestroyImage(ximage);

    return frame;
}

std::shared_ptr<Frame> X11ScreenCaptureEngine::ConvertXImageToFrame(XImage *ximage)
{
    if (!ximage) {
        return nullptr;
    }

    auto frame = std::make_shared<Frame>();

    // Set frame properties
    frame->video_info.width = ximage->width;
    frame->video_info.height = ximage->height;
    frame->video_info.framerate = config_.frame_rate;

    // Determine format based on XImage properties (raw data from X11)
    FrameFormat detected_format = FrameFormat::UNKNOWN;
    if (ximage->depth == 24 && ximage->bits_per_pixel == 32) {
        // Check color masks to determine exact format
        if (ximage->red_mask == 0x00FF0000 && ximage->green_mask == 0x0000FF00 && ximage->blue_mask == 0x000000FF) {
            detected_format = FrameFormat::BGRA32;
        } else if (ximage->red_mask == 0x000000FF && ximage->green_mask == 0x0000FF00 &&
                   ximage->blue_mask == 0x00FF0000) {
            detected_format = FrameFormat::RGBA32;
        }
    }

    // Set format to the detected raw format from X11
    frame->format = detected_format;

    // Calculate frame size
    size_t frame_size = ximage->width * ximage->height * 4; // 4 bytes per pixel
    frame->SetSize(frame_size);

    LOG_DEBUG("Direct raw format output: depth=%d, bits_per_pixel=%d, format=%s", ximage->depth, ximage->bits_per_pixel,
              detected_format == FrameFormat::BGRA32   ? "BGRA32"
              : detected_format == FrameFormat::RGBA32 ? "RGBA32"
                                                       : "UNKNOWN");

    // Direct memory copy - no conversion needed
    if (detected_format != FrameFormat::UNKNOWN && ximage->bytes_per_line == ximage->width * 4) {
        // Direct memory copy for optimal performance
        std::memcpy(frame->Data(), ximage->data, frame_size);
        LOG_DEBUG("Zero-copy direct memory transfer completed for %d pixels", ximage->width * ximage->height);
    } else {
        // Fallback: copy row by row if memory layout is not contiguous
        uint8_t *dst = (uint8_t *)frame->Data();
        const uint8_t *src = (const uint8_t *)ximage->data;
        const int row_bytes = ximage->width * 4;

        for (int y = 0; y < ximage->height; ++y) {
            std::memcpy(dst + y * row_bytes, src + y * ximage->bytes_per_line, row_bytes);
        }
        LOG_DEBUG("Row-by-row memory copy completed for %dx%d image", ximage->width, ximage->height);
    }

    return frame;
}

// Note: Format conversion functions removed - now using direct raw format output

void X11ScreenCaptureEngine::CaptureCursor(std::shared_ptr<Frame> frame)
{
    // Cursor capture not available in demo mode
    // In a real implementation, this would overlay cursor image onto the frame
    (void)frame; // Suppress unused parameter warning
}

void X11ScreenCaptureEngine::Cleanup()
{
    LOG_DEBUG("Cleaning up X11 screen capture resources");

    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
        screen_ = nullptr;
        root_window_ = 0;
    }

    LOG_DEBUG("X11 screen capture cleanup completed");
}

bool X11ScreenCaptureEngine::GetScreenGeometry(uint32_t monitor_index, int &x, int &y, uint32_t &width,
                                               uint32_t &height)
{
    // Simulate screen geometry for demo
    if (monitor_index == 0) {
        x = 0;
        y = 0;
        width = 1920;
        height = 1080;
        return true;
    }
    return false;
}

} // namespace lmshao::remotedesk

#endif // __linux__