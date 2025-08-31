/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "video_scaler.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "../log/remote_desk_log.h"

namespace lmshao::remotedesk {

VideoScaler::VideoScaler(const VideoScalerConfig &config) : config_(config)
{
    last_stats_time_ = std::chrono::steady_clock::now();
    LOG_DEBUG("VideoScaler created with target resolution %ux%u, algorithm=%d, maintain_aspect_ratio=%s",
              config_.target_width, config_.target_height, static_cast<int>(config_.algorithm),
              config_.maintain_aspect_ratio ? "true" : "false");
}

VideoScaler::~VideoScaler()
{
    LOG_DEBUG("VideoScaler destructor called");
    Stop();
    LOG_DEBUG("VideoScaler destroyed");
}

bool VideoScaler::SetTargetResolution(uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (width == 0 || height == 0) {
        LOG_ERROR("Invalid target resolution: %ux%u (width or height is zero)", width, height);
        return false;
    }

    LOG_INFO("Setting target resolution from %ux%u to %ux%u", config_.target_width, config_.target_height, width,
             height);
    config_.target_width = width;
    config_.target_height = height;
    return true;
}

bool VideoScaler::SetScalingAlgorithm(ScalingAlgorithm algorithm)
{
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Setting scaling algorithm from %d to %d", static_cast<int>(config_.algorithm),
             static_cast<int>(algorithm));
    config_.algorithm = algorithm;
    return true;
}

bool VideoScaler::Initialize()
{
    std::lock_guard<std::mutex> lock(mutex_);

    LOG_DEBUG("Initializing VideoScaler");

    // Validate configuration
    if (config_.target_width == 0 || config_.target_height == 0) {
        LOG_ERROR("Invalid configuration: target resolution %ux%u is invalid", config_.target_width,
                  config_.target_height);
        return false;
    }

    LOG_INFO("VideoScaler initialized successfully with target resolution %ux%u, algorithm=%d", config_.target_width,
             config_.target_height, static_cast<int>(config_.algorithm));
    return true;
}

void VideoScaler::OnFrame(std::shared_ptr<Frame> frame)
{
    if (!frame || !frame->IsValid() || !frame->IsVideo()) {
        LOG_WARN("Received invalid frame: frame=%p, valid=%s, video=%s", frame.get(),
                 frame ? (frame->IsValid() ? "true" : "false") : "N/A",
                 frame ? (frame->IsVideo() ? "true" : "false") : "N/A");
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.frames_dropped++;
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    // Check if scaling is needed
    if (!IsScalingNeeded(frame->width(), frame->height())) {
        LOG_DEBUG("No scaling needed for frame %ux%u (matches target), forwarding directly", frame->width(),
                  frame->height());
        // Forward frame without scaling
        DeliverFrame(frame);
        return;
    }

    // Scale the frame
    LOG_DEBUG("Scaling frame from %ux%u to target %ux%u", frame->width(), frame->height(), config_.target_width,
              config_.target_height);
    auto scaled_frame = ScaleFrame(frame);

    auto end_time = std::chrono::steady_clock::now();
    auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (scaled_frame) {
        LOG_DEBUG("Frame scaled successfully from %ux%u to %ux%u in %lldms", frame->width(), frame->height(),
                  scaled_frame->width(), scaled_frame->height(), processing_time.count());
        UpdateStats(frame->width(), frame->height(), scaled_frame->width(), scaled_frame->height(), processing_time);
        // Forward scaled frame
        DeliverFrame(scaled_frame);
    } else {
        LOG_ERROR("Failed to scale frame from %ux%u to %ux%u", frame->width(), frame->height(), config_.target_width,
                  config_.target_height);
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.frames_dropped++;
    }
}

std::shared_ptr<Frame> VideoScaler::ScaleFrame(std::shared_ptr<Frame> input_frame)
{
    if (!input_frame || !input_frame->IsValid()) {
        LOG_ERROR("ScaleFrame: Invalid input frame");
        return nullptr;
    }

    // Calculate target dimensions
    auto [target_width, target_height] = CalculateTargetDimensions(input_frame->width(), input_frame->height());

    LOG_DEBUG("ScaleFrame: Input %ux%u -> Target %ux%u, format=%d", input_frame->width(), input_frame->height(),
              target_width, target_height, static_cast<int>(input_frame->format));

    // Create output frame
    auto output_frame = std::make_shared<Frame>();
    output_frame->format = input_frame->format;
    output_frame->timestamp = input_frame->timestamp;
    output_frame->width() = target_width;
    output_frame->height() = target_height;
    output_frame->video_info.framerate = input_frame->video_info.framerate;
    output_frame->video_info.is_keyframe = input_frame->video_info.is_keyframe;

    // Calculate bytes per pixel based on format
    uint32_t bytes_per_pixel = 4; // Default for BGRA32/RGBA32
    switch (input_frame->format) {
        case FrameFormat::RGB24:
        case FrameFormat::BGR24:
            bytes_per_pixel = 3;
            break;
        case FrameFormat::RGBA32:
        case FrameFormat::BGRA32:
            bytes_per_pixel = 4;
            break;
        case FrameFormat::I420:
            // For I420, we need special handling
            bytes_per_pixel = 3; // Simplified calculation
            break;
        default:
            bytes_per_pixel = 4;
            break;
    }

    // Calculate output frame size
    size_t output_size = target_width * target_height * bytes_per_pixel;
    LOG_DEBUG("ScaleFrame: Allocating output frame: %ux%u, %u bytes_per_pixel, total size: %zu bytes", target_width,
              target_height, bytes_per_pixel, output_size);
    output_frame->SetCapacity(output_size);
    output_frame->SetSize(output_size);
    output_frame->stride = target_width * bytes_per_pixel;

    // Perform simple bilinear scaling (software implementation)
    // Note: In production, you might want to use libswscale or hardware acceleration
    if (input_frame->format == FrameFormat::BGRA32 || input_frame->format == FrameFormat::RGBA32) {
        LOG_DEBUG("ScaleFrame: Performing bilinear scaling for format %s",
                  input_frame->format == FrameFormat::BGRA32 ? "BGRA32" : "RGBA32");
        PerformBilinearScaling(input_frame, output_frame);
    } else {
        LOG_ERROR("ScaleFrame: Unsupported pixel format %d for scaling", static_cast<int>(input_frame->format));
        // For other formats, implement specific scaling or use fallback
        return nullptr;
    }

    LOG_DEBUG("ScaleFrame: Successfully scaled frame from %ux%u to %ux%u", input_frame->width(), input_frame->height(),
              target_width, target_height);

    return output_frame;
}

void VideoScaler::PerformBilinearScaling(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output)
{
    const uint8_t *src = input->data();
    uint8_t *dst = output->data();

    uint32_t src_width = input->width();
    uint32_t src_height = input->height();
    uint32_t dst_width = output->width();
    uint32_t dst_height = output->height();

    float x_ratio = static_cast<float>(src_width) / dst_width;
    float y_ratio = static_cast<float>(src_height) / dst_height;

    for (uint32_t y = 0; y < dst_height; ++y) {
        for (uint32_t x = 0; x < dst_width; ++x) {
            // Calculate source coordinates
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;

            // Get integer coordinates
            uint32_t x1 = static_cast<uint32_t>(src_x);
            uint32_t y1 = static_cast<uint32_t>(src_y);
            uint32_t x2 = std::min(x1 + 1, src_width - 1);
            uint32_t y2 = std::min(y1 + 1, src_height - 1);

            // Calculate fractional parts
            float dx = src_x - x1;
            float dy = src_y - y1;

            // Get pixel positions
            size_t pos_tl = (y1 * src_width + x1) * 4; // Top-left
            size_t pos_tr = (y1 * src_width + x2) * 4; // Top-right
            size_t pos_bl = (y2 * src_width + x1) * 4; // Bottom-left
            size_t pos_br = (y2 * src_width + x2) * 4; // Bottom-right

            size_t dst_pos = (y * dst_width + x) * 4;

            // Perform bilinear interpolation for each channel (BGRA)
            for (int c = 0; c < 4; ++c) {
                float tl = src[pos_tl + c];
                float tr = src[pos_tr + c];
                float bl = src[pos_bl + c];
                float br = src[pos_br + c];

                float top = tl + dx * (tr - tl);
                float bottom = bl + dx * (br - bl);
                float result = top + dy * (bottom - top);

                dst[dst_pos + c] = static_cast<uint8_t>(std::clamp(result, 0.0f, 255.0f));
            }
        }
    }
}

std::pair<uint32_t, uint32_t> VideoScaler::CalculateTargetDimensions(uint32_t input_width, uint32_t input_height) const
{
    if (!config_.maintain_aspect_ratio) {
        return {config_.target_width, config_.target_height};
    }

    // Calculate aspect ratios
    float input_aspect = static_cast<float>(input_width) / input_height;
    float target_aspect = static_cast<float>(config_.target_width) / config_.target_height;

    uint32_t width, height;

    if (input_aspect > target_aspect) {
        // Input is wider, fit to width
        width = config_.target_width;
        height = static_cast<uint32_t>(config_.target_width / input_aspect);
    } else {
        // Input is taller, fit to height
        width = static_cast<uint32_t>(config_.target_height * input_aspect);
        height = config_.target_height;
    }

    // Ensure dimensions are even numbers (required for many video codecs)
    width = (width + 1) & ~1;
    height = (height + 1) & ~1;

    return {width, height};
}

bool VideoScaler::IsScalingNeeded(uint32_t input_width, uint32_t input_height) const
{
    auto [target_width, target_height] = CalculateTargetDimensions(input_width, input_height);
    return (input_width != target_width) || (input_height != target_height);
}

void VideoScaler::UpdateStats(uint32_t input_width, uint32_t input_height, uint32_t output_width,
                              uint32_t output_height, std::chrono::milliseconds processing_time)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);

    stats_.frames_processed++;
    stats_.input_width = input_width;
    stats_.input_height = input_height;
    stats_.output_width = output_width;
    stats_.output_height = output_height;

    // Update average processing time using exponential moving average
    if (stats_.frames_processed == 1) {
        stats_.avg_scaling_time = processing_time;
    } else {
        auto new_avg = stats_.avg_scaling_time.count() * 0.9 + processing_time.count() * 0.1;
        stats_.avg_scaling_time = std::chrono::milliseconds(static_cast<long long>(new_avg));
    }

    // Log statistics every 100 frames
    if (stats_.frames_processed % 100 == 0) {
        LOG_INFO("VideoScaler stats: %llu frames processed, %llu dropped, avg time: %lldms, current: %ux%u->%ux%u",
                 static_cast<unsigned long long>(stats_.frames_processed),
                 static_cast<unsigned long long>(stats_.frames_dropped), stats_.avg_scaling_time.count(), input_width,
                 input_height, output_width, output_height);
    }
}

VideoScaler::ScalingStats VideoScaler::GetStats() const
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

} // namespace lmshao::remotedesk