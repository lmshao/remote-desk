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

namespace lmshao::remotedesk {

DesktopDuplicationScreenCaptureEngine::DesktopDuplicationScreenCaptureEngine()
{
    frame_interval_ = std::chrono::milliseconds(1000 / 30); // Default 30 FPS
}

DesktopDuplicationScreenCaptureEngine::~DesktopDuplicationScreenCaptureEngine()
{
    Stop();
    Cleanup();
}

CaptureResult DesktopDuplicationScreenCaptureEngine::Initialize(const ScreenCaptureConfig &config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (is_running_) {
        LOG_ERROR("Cannot initialize while capture is running");
        return CaptureResult::ErrorInitialization;
    }

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

    should_stop_ = false;
    capture_thread_ = std::make_unique<std::thread>(&WindowsScreenCaptureEngine::CaptureThreadProc, this);
    is_running_ = true;

    return CaptureResult::Success;
}

void DesktopDuplicationScreenCaptureEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!is_running_) {
        return;
    }

    should_stop_ = true;
    if (capture_thread_ && capture_thread_->joinable()) {
        capture_thread_->join();
    }
    capture_thread_.reset();
    is_running_ = false;
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
    D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                          D3D_FEATURE_LEVEL_10_0};

    D3D_FEATURE_LEVEL feature_level;
    HRESULT hr =
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels, ARRAYSIZE(feature_levels),
                          D3D11_SDK_VERSION, &d3d_device_, &feature_level, &d3d_context_);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D11 device");
        return CaptureResult::ErrorInitialization;
    }

    return CaptureResult::Success;
}

CaptureResult DesktopDuplicationScreenCaptureEngine::InitializeDesktopDuplication()
{
    // Get DXGI device
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    HRESULT hr = d3d_device_.As(&dxgi_device);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI device");
        return CaptureResult::ErrorInitialization;
    }

    // Get adapter
    hr = dxgi_device->GetAdapter(&dxgi_adapter_);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI adapter");
        return CaptureResult::ErrorInitialization;
    }

    // Get output
    hr = dxgi_adapter_->EnumOutputs(config_.monitor_index,
                                    reinterpret_cast<IDXGIOutput **>(dxgi_output_.GetAddressOf()));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to enumerate DXGI output");
        return CaptureResult::ErrorInitialization;
    }

    // Get desktop duplication
    hr = dxgi_output_->DuplicateOutput(d3d_device_.Get(), &desktop_duplication_);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
            LOG_ERROR("Desktop duplication not available (may be in use by another application)");
        } else {
            LOG_ERROR("Failed to create desktop duplication");
        }
        return CaptureResult::ErrorInitialization;
    }

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

    HRESULT hr = desktop_duplication_->AcquireNextFrame(0, &frame_info, &desktop_resource);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return CaptureResult::Success; // No new frame available
        }
        return CaptureResult::ErrorUnknown;
    }

    // Get surface from resource
    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    hr = desktop_resource.As(&surface);
    if (FAILED(hr)) {
        desktop_duplication_->ReleaseFrame();
        return CaptureResult::ErrorUnknown;
    }

    // Convert surface to frame
    auto frame = ConvertSurfaceToFrame(surface);
    if (frame && frame_callback_) {
        frame_callback_(frame);
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
        return nullptr;
    }

    DXGI_MAPPED_RECT mapped_rect;
    hr = surface->Map(&mapped_rect, DXGI_MAP_READ);
    if (FAILED(hr)) {
        return nullptr;
    }

    // Create frame
    auto frame = std::make_shared<Frame>();
    frame->width = desc.Width;
    frame->height = desc.Height;
    frame->format = FrameFormat::BGRA;
    frame->timestamp =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();

    // Copy pixel data
    size_t data_size = desc.Height * mapped_rect.Pitch;
    frame->data.resize(data_size);
    memcpy(frame->data.data(), mapped_rect.pBits, data_size);
    frame->stride = mapped_rect.Pitch;

    surface->Unmap();
    return frame;
}

void DesktopDuplicationScreenCaptureEngine::Cleanup()
{
    desktop_duplication_.Reset();
    dxgi_output_.Reset();
    dxgi_adapter_.Reset();
    d3d_context_.Reset();
    d3d_device_.Reset();
}

void DesktopDuplicationScreenCaptureEngine::LOG_ERROR(const std::string &error)
{
    last_error_ = error;
}

} // namespace lmshao::remotedesk

#endif // _WIN32