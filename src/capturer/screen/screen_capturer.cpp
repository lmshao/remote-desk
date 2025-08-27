/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "screen_capturer.h"

#include <stdexcept>

#include "../../core/frame.h"

namespace lmshao::remotedesk {

ScreenCapturer::ScreenCapturer(const ScreenCaptureConfig &config, ScreenCaptureEngineFactory::Technology technology)
    : config_(config), technology_(technology), initialized_(false)
{
    // Create the technology-specific engine
    engine_ = ScreenCaptureEngineFactory::CreateEngine(technology_);
    if (!engine_) {
        throw std::runtime_error("Failed to create screen capture engine for technology: " +
                                 ScreenCaptureEngineFactory::GetTechnologyName(technology_));
    }
}

ScreenCapturer::~ScreenCapturer()
{
    Stop();
}

bool ScreenCapturer::Initialize()
{
    if (!engine_) {
        return false;
    }

    auto result = engine_->Initialize(config_);
    initialized_ = (result == CaptureResult::Success);

    if (initialized_) {
        // Set up frame callback to forward frames to sinks
        engine_->SetFrameCallback([this](std::shared_ptr<Frame> frame) { OnFrameCaptured(frame); });
    }

    return initialized_;
}

bool ScreenCapturer::Start()
{
    if (!engine_ || !initialized_) {
        return false;
    }

    auto result = engine_->Start();
    return (result == CaptureResult::Success);
}

void ScreenCapturer::Stop()
{
    if (engine_) {
        engine_->Stop();
    }
}

bool ScreenCapturer::IsRunning() const
{
    if (!engine_) {
        return false;
    }
    return engine_->IsRunning();
}

std::vector<ScreenInfo> ScreenCapturer::GetAvailableScreens() const
{
    if (!engine_) {
        return {};
    }
    return engine_->GetAvailableScreens();
}

const ScreenCaptureConfig &ScreenCapturer::GetConfig() const
{
    return config_;
}

CaptureResult ScreenCapturer::UpdateConfig(const ScreenCaptureConfig &config)
{
    config_ = config;

    if (!engine_) {
        return CaptureResult::ErrorInitialization;
    }

    return engine_->UpdateConfig(config_);
}

std::string ScreenCapturer::GetTechnologyName() const
{
    return ScreenCaptureEngineFactory::GetTechnologyName(technology_);
}

void ScreenCapturer::OnFrameCaptured(std::shared_ptr<Frame> frame)
{
    if (!frame) {
        return;
    }

    // Forward the frame to all connected sinks using the base class method
    DeliverFrame(frame);
}

} // namespace lmshao::remotedesk