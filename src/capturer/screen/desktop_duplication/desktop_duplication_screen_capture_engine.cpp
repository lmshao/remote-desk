/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef _WIN32

#include "desktop_duplication_screen_capture_engine.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "../../../log/remote_desk_log.h"

namespace lmshao::remotedesk {

DesktopDuplicationScreenCaptureEngine::DesktopDuplicationScreenCaptureEngine()
    : capture_thread_(nullptr), should_stop_(false), last_frame_time_(),
      frame_interval_(std::chrono::milliseconds(33)), // Default 30 FPS
      capture_x_(0), capture_y_(0), capture_width_(0), capture_height_(0)
{
    LOG_DEBUG("DesktopDuplicationScreenCaptureEngine created");
    frame_interval_ = std::chrono::milliseconds(1000 / 30); // Default 30 FPS
}

DesktopDuplicationScreenCaptureEngine::~DesktopDuplicationScreenCaptureEngine()
{
    Stop();
    Cleanup();
    LOG_DEBUG("DesktopDuplicationScreenCaptureEngine destroyed");
}

CaptureResult DesktopDuplicationScreenCaptureEngine::Initialize(const ScreenCaptureConfig &config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_running_) {
        LOG_ERROR("Cannot initialize while capture is running");
        return CaptureResult::ErrorInitialization;
    }

    LOG_DEBUG("Initializing Desktop Duplication screen capture engine");

    config_ = config;
    frame_interval_ = std::chrono::milliseconds(1000 / config_.frame_rate);

    // Initialize D3D11
    auto result = InitializeD3D();
    if (result != CaptureResult::Success) {
        return result;
    }

    // Initialize Desktop Duplication
    result = InitializeDesktopDuplication();
    if (result != CaptureResult::Success) {
        Cleanup();
        return result;
    }

    LOG_DEBUG("Desktop Duplication screen capture engine initialized successfully");
    return CaptureResult::Success;
}

CaptureResult DesktopDuplicationScreenCaptureEngine::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_running_) {
        return CaptureResult::Success;
    }

    if (!desktop_duplication_) {
        LOG_ERROR("Desktop duplication not initialized");
        return CaptureResult::ErrorInitialization;
    }

    LOG_DEBUG("Starting Desktop Duplication screen capture");

    should_stop_ = false;
    capture_thread_ = std::make_unique<std::thread>(&DesktopDuplicationScreenCaptureEngine::CaptureThreadProc, this);
    is_running_ = true;

    LOG_DEBUG("Desktop Duplication screen capture started");
    return CaptureResult::Success;
}

void DesktopDuplicationScreenCaptureEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_running_) {
        return;
    }

    LOG_DEBUG("Stopping Desktop Duplication screen capture");

    should_stop_ = true;
    if (capture_thread_ && capture_thread_->joinable()) {
        capture_thread_->join();
    }
    capture_thread_.reset();
    is_running_ = false;

    LOG_DEBUG("Desktop Duplication screen capture stopped");
}

bool DesktopDuplicationScreenCaptureEngine::IsRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return is_running_;
}

std::vector<ScreenInfo> DesktopDuplicationScreenCaptureEngine::GetAvailableScreens() const
{
    std::vector<ScreenInfo> screens;

    // Enumerate displays using DXGI
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), &factory))) {
        LOG_ERROR("Failed to create DXGI factory for screen enumeration");
        return screens;
    }

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapter_idx = 0; factory->EnumAdapters1(adapter_idx, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_idx) {
        Microsoft::WRL::ComPtr<IDXGIOutput> output;
        for (UINT output_idx = 0; adapter->EnumOutputs(output_idx, &output) != DXGI_ERROR_NOT_FOUND; ++output_idx) {
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(output->GetDesc(&desc))) {
                ScreenInfo info;
                info.width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
                info.height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
                info.x = desc.DesktopCoordinates.left;
                info.y = desc.DesktopCoordinates.top;
                info.bits_per_pixel = 32; // Assume 32-bit BGRA

                // Convert wide string to string
                int size = WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, nullptr, 0, nullptr, nullptr);
                if (size > 0) {
                    std::string name(size - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, -1, &name[0], size, nullptr, nullptr);
                    info.name = name;
                }

                info.is_primary = (adapter_idx == 0 && output_idx == 0);
                screens.push_back(info);
            }
        }
    }

    LOG_DEBUG("Available screens: %zu", screens.size());
    return screens;
}

void DesktopDuplicationScreenCaptureEngine::SetFrameCallback(std::function<void(std::shared_ptr<Frame>)> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    frame_callback_ = callback;
}

const ScreenCaptureConfig &DesktopDuplicationScreenCaptureEngine::GetConfig() const
{
    return config_;
}

CaptureResult DesktopDuplicationScreenCaptureEngine::UpdateConfig(const ScreenCaptureConfig &config)
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

CaptureResult DesktopDuplicationScreenCaptureEngine::InitializeD3D()
{
    LOG_DEBUG("Initializing D3D11 device");

    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                          D3D_FEATURE_LEVEL_10_0};

    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr =
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels, ARRAYSIZE(feature_levels),
                          D3D11_SDK_VERSION, &d3d_device_, &feature_level, &d3d_context_);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D11 device (HRESULT: 0x%08X)", hr);
        return HResultToCaptureResult(hr, "D3D11CreateDevice");
    }

    LOG_DEBUG("D3D11 device created successfully with feature level 0x%04X", feature_level);
    return CaptureResult::Success;
}

CaptureResult DesktopDuplicationScreenCaptureEngine::InitializeDesktopDuplication()
{
    LOG_DEBUG("Initializing Desktop Duplication API");

    // Get DXGI device
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = d3d_device_.As(&dxgi_device);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI device (HRESULT: 0x%08X)", hr);
        return HResultToCaptureResult(hr, "Get DXGI device");
    }

    // Get adapter
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgi_device->GetAdapter(&adapter);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI adapter (HRESULT: 0x%08X)", hr);
        return HResultToCaptureResult(hr, "Get DXGI adapter");
    }

    // Query for IDXGIAdapter1
    hr = adapter.As(&dxgi_adapter_);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to query IDXGIAdapter1 (HRESULT: 0x%08X)", hr);
        return HResultToCaptureResult(hr, "Query IDXGIAdapter1");
    }

    // Get output
    hr = dxgi_adapter_->EnumOutputs(config_.monitor_index,
                                    reinterpret_cast<IDXGIOutput **>(dxgi_output_.GetAddressOf()));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to enumerate DXGI output %d (HRESULT: 0x%08X)", config_.monitor_index, hr);
        return HResultToCaptureResult(hr, "Enumerate DXGI output");
    }

    // Get screen geometry
    DXGI_OUTPUT_DESC output_desc;
    hr = dxgi_output_->GetDesc(&output_desc);
    if (SUCCEEDED(hr)) {
        // Set capture area based on config or use full screen
        if (config_.width > 0 && config_.height > 0) {
            capture_width_ = config_.width;
            capture_height_ = config_.height;
            capture_x_ = config_.offset_x;
            capture_y_ = config_.offset_y;
        } else {
            capture_width_ = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
            capture_height_ = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;
            capture_x_ = 0;
            capture_y_ = 0;
        }
        LOG_DEBUG("Capture region: %dx%d at (%d,%d)", capture_width_, capture_height_, capture_x_, capture_y_);
    }

    // Get desktop duplication
    hr = dxgi_output_->DuplicateOutput(d3d_device_.Get(), &desktop_duplication_);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
            LOG_ERROR("Desktop duplication not available (may be in use by another application)");
        } else {
            LOG_ERROR("Failed to create desktop duplication (HRESULT: 0x%08X)", hr);
        }
        return HResultToCaptureResult(hr, "Create desktop duplication");
    }

    LOG_DEBUG("Desktop Duplication API initialized successfully");
    return CaptureResult::Success;
}

void DesktopDuplicationScreenCaptureEngine::CaptureThreadProc()
{
    last_frame_time_ = std::chrono::steady_clock::now();

    while (!should_stop_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - last_frame_time_;

        if (elapsed >= frame_interval_) {
            CaptureFrame();
            last_frame_time_ = now;
        } else {
            // Sleep for a short time to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

CaptureResult DesktopDuplicationScreenCaptureEngine::CaptureFrame()
{
    if (!desktop_duplication_) {
        return CaptureResult::ErrorInitialization;
    }

    Microsoft::WRL::ComPtr<IDXGIResource> desktop_resource;
    DXGI_OUTDUPL_FRAME_INFO frame_info;

    HRESULT hr = desktop_duplication_->AcquireNextFrame(1000, &frame_info, &desktop_resource); // 1 second timeout
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            // No new frame available, this is normal
            return CaptureResult::Success;
        } else if (hr == DXGI_ERROR_ACCESS_LOST) {
            LOG_WARN("Desktop duplication access lost, need to recreate");
            return CaptureResult::ErrorAccessDenied;
        } else {
            LOG_ERROR("Failed to acquire next frame (HRESULT: 0x%08X)", hr);
            return CaptureResult::ErrorUnknown;
        }
    }

    // Check if there was any desktop update
    if (frame_info.LastPresentTime.QuadPart == 0) {
        // No desktop update
        desktop_duplication_->ReleaseFrame();
        return CaptureResult::Success;
    }

    // Get surface from resource
    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    hr = desktop_resource.As(&surface);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get surface from resource (HRESULT: 0x%08X)", hr);
        desktop_duplication_->ReleaseFrame();
        return CaptureResult::ErrorUnknown;
    }

    // Convert surface to frame
    auto frame = ConvertSurfaceToFrame(surface);
    if (frame && frame_callback_) {
        frame_callback_(frame);
    } else if (!frame) {
        LOG_ERROR("Failed to convert surface to frame");
    }

    desktop_duplication_->ReleaseFrame();
    return CaptureResult::Success;
}

std::shared_ptr<Frame>
DesktopDuplicationScreenCaptureEngine::ConvertSurfaceToFrame(Microsoft::WRL::ComPtr<IDXGISurface> surface)
{
    DXGI_SURFACE_DESC desc;
    HRESULT hr = surface->GetDesc(&desc);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get surface description (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    // Get ID3D11Texture2D from the surface
    Microsoft::WRL::ComPtr<ID3D11Texture2D> desktop_texture;
    hr = surface->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(desktop_texture.GetAddressOf()));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to query ID3D11Texture2D from surface (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    // Get texture description
    D3D11_TEXTURE2D_DESC texture_desc;
    desktop_texture->GetDesc(&texture_desc);

    // Create staging texture for CPU access
    D3D11_TEXTURE2D_DESC staging_desc = texture_desc;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.BindFlags = 0;
    staging_desc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_texture;
    hr = d3d_device_->CreateTexture2D(&staging_desc, nullptr, &staging_texture);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create staging texture (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    // Copy desktop texture to staging texture
    d3d_context_->CopyResource(staging_texture.Get(), desktop_texture.Get());

    // Map the staging texture
    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    hr = d3d_context_->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped_resource);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to map staging texture (HRESULT: 0x%08X)", hr);
        return nullptr;
    }

    // Create frame
    auto frame = std::make_shared<Frame>();
    frame->width() = desc.Width;
    frame->height() = desc.Height;
    frame->format = FrameFormat::BGRA32;
    frame->timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();

    // Calculate data size (BGRA format uses 4 bytes per pixel)
    size_t bytes_per_pixel = 4;
    size_t row_size = desc.Width * bytes_per_pixel;
    size_t total_size = desc.Height * row_size;

    // Set frame capacity and size
    frame->SetCapacity(total_size);
    frame->SetSize(total_size);
    frame->stride = mapped_resource.RowPitch;

    // Copy pixel data row by row if stride doesn't match width
    uint8_t *dst = frame->data();
    const uint8_t *src = static_cast<const uint8_t *>(mapped_resource.pData);

    if (mapped_resource.RowPitch == row_size) {
        // Direct copy if no padding
        memcpy(dst, src, total_size);
    } else {
        // Row-by-row copy to handle stride
        for (uint32_t y = 0; y < desc.Height; ++y) {
            memcpy(dst + y * row_size, src + y * mapped_resource.RowPitch, row_size);
        }
    }

    // Unmap the staging texture
    d3d_context_->Unmap(staging_texture.Get(), 0);

    LOG_DEBUG("Successfully converted surface to frame (%dx%d, format: BGRA32)", desc.Width, desc.Height);
    return frame;
}

void DesktopDuplicationScreenCaptureEngine::Cleanup()
{
    LOG_DEBUG("Cleaning up Desktop Duplication resources");
    desktop_duplication_.Reset();
    dxgi_output_.Reset();
    dxgi_adapter_.Reset();
    d3d_context_.Reset();
    d3d_device_.Reset();
}

CaptureResult DesktopDuplicationScreenCaptureEngine::HResultToCaptureResult(HRESULT hr, const std::string &context)
{
    switch (hr) {
        case S_OK:
            return CaptureResult::Success;
        case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
            return CaptureResult::ErrorAccessDenied;
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
            return CaptureResult::ErrorInitialization;
        case E_ACCESSDENIED:
            return CaptureResult::ErrorAccessDenied;
        case DXGI_ERROR_WAIT_TIMEOUT:
            return CaptureResult::ErrorTimeout;
        default:
            return CaptureResult::ErrorUnknown;
    }
}

void DesktopDuplicationScreenCaptureEngine::CaptureCursor(std::shared_ptr<Frame> frame)
{
    // TODO: Implement cursor capture using GetCursorInfo and overlaying on frame
    // This would require additional Win32 API calls and image composition
    // For now, cursor capture is not implemented
}

bool DesktopDuplicationScreenCaptureEngine::GetScreenGeometry(uint32_t monitor_index, int &x, int &y, uint32_t &width,
                                                              uint32_t &height)
{
    // Get available screens and check if monitor_index is valid
    auto screens = GetAvailableScreens();
    if (monitor_index >= screens.size()) {
        return false;
    }

    const auto &screen = screens[monitor_index];
    x = screen.x;
    y = screen.y;
    width = screen.width;
    height = screen.height;
    return true;
}

} // namespace lmshao::remotedesk

#endif // _WIN32