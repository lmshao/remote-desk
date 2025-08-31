/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "pixel_format_converter.h"

#include <algorithm>
#include <cstring>

namespace lmshao::remotedesk {

PixelFormatConverter::PixelFormatConverter(const PixelFormatConverterConfig &config) : config_(config) {}

PixelFormatConverter::~PixelFormatConverter()
{
    Stop();
}

bool PixelFormatConverter::SetOutputFormat(FrameFormat format)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate format compatibility
    if (!IsFormatSupported(format)) {
        return false;
    }

    config_.output_format = format;
    return true;
}

bool PixelFormatConverter::Initialize()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate input and output formats
    if (!IsFormatSupported(config_.input_format) || !IsFormatSupported(config_.output_format)) {
        return false;
    }

    return true;
}

void PixelFormatConverter::OnFrame(std::shared_ptr<Frame> frame)
{
    if (!frame || !frame->IsValid() || !frame->IsVideo()) {
        return;
    }

    // Check if conversion is needed
    if (frame->format == config_.output_format) {
        // No conversion needed, forward frame directly
        DeliverFrame(frame);
        return;
    }

    // Convert frame
    auto converted_frame = ConvertFrame(frame);
    if (converted_frame) {
        DeliverFrame(converted_frame);
    }
}

std::shared_ptr<Frame> PixelFormatConverter::ConvertFrame(std::shared_ptr<Frame> input_frame)
{
    if (!input_frame || !input_frame->IsValid()) {
        return nullptr;
    }

    // Calculate output frame size
    size_t output_size = CalculateOutputFrameSize(input_frame->width(), input_frame->height(), config_.output_format);

    // Create output frame with correct size
    auto output_frame = std::make_shared<Frame>(output_size);
    output_frame->format = config_.output_format;
    output_frame->timestamp = input_frame->timestamp;
    output_frame->width() = input_frame->width();
    output_frame->height() = input_frame->height();
    output_frame->video_info.framerate = input_frame->video_info.framerate;
    output_frame->video_info.is_keyframe = input_frame->video_info.is_keyframe;

    // Set frame size
    output_frame->SetSize(output_size);

    // Perform format conversion
    bool success = false;
    switch (input_frame->format) {
        case FrameFormat::BGRA32:
            success = ConvertFromBGRA32(input_frame, output_frame);
            break;
        case FrameFormat::RGBA32:
            success = ConvertFromRGBA32(input_frame, output_frame);
            break;
        case FrameFormat::RGB24:
            success = ConvertFromRGB24(input_frame, output_frame);
            break;
        case FrameFormat::BGR24:
            success = ConvertFromBGR24(input_frame, output_frame);
            break;
        default:
            return nullptr; // Unsupported input format
    }

    return success ? output_frame : nullptr;
}

bool PixelFormatConverter::ConvertFromBGRA32(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output)
{
    const uint8_t *src = input->data();
    uint8_t *dst = output->data();
    uint32_t width = input->width();
    uint32_t height = input->height();

    switch (config_.output_format) {
        case FrameFormat::RGBA32:
            return ConvertBGRA32ToRGBA32(src, dst, width, height);
        case FrameFormat::RGB24:
            return ConvertBGRA32ToRGB24(src, dst, width, height);
        case FrameFormat::BGR24:
            return ConvertBGRA32ToBGR24(src, dst, width, height);
        case FrameFormat::I420:
            return ConvertBGRA32ToI420(src, dst, width, height);
        default:
            return false;
    }
}

bool PixelFormatConverter::ConvertFromRGBA32(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output)
{
    const uint8_t *src = input->data();
    uint8_t *dst = output->data();
    uint32_t width = input->width();
    uint32_t height = input->height();

    switch (config_.output_format) {
        case FrameFormat::BGRA32:
            return ConvertRGBA32ToBGRA32(src, dst, width, height);
        case FrameFormat::RGB24:
            return ConvertRGBA32ToRGB24(src, dst, width, height);
        case FrameFormat::BGR24:
            return ConvertRGBA32ToBGR24(src, dst, width, height);
        case FrameFormat::I420:
            return ConvertRGBA32ToI420(src, dst, width, height);
        default:
            return false;
    }
}

bool PixelFormatConverter::ConvertFromRGB24(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output)
{
    const uint8_t *src = input->data();
    uint8_t *dst = output->data();
    uint32_t width = input->width();
    uint32_t height = input->height();

    switch (config_.output_format) {
        case FrameFormat::BGR24:
            return ConvertRGB24ToBGR24(src, dst, width, height);
        case FrameFormat::RGBA32:
            return ConvertRGB24ToRGBA32(src, dst, width, height);
        case FrameFormat::BGRA32:
            return ConvertRGB24ToBGRA32(src, dst, width, height);
        case FrameFormat::I420:
            return ConvertRGB24ToI420(src, dst, width, height);
        default:
            return false;
    }
}

bool PixelFormatConverter::ConvertFromBGR24(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output)
{
    const uint8_t *src = input->data();
    uint8_t *dst = output->data();
    uint32_t width = input->width();
    uint32_t height = input->height();

    switch (config_.output_format) {
        case FrameFormat::RGB24:
            return ConvertBGR24ToRGB24(src, dst, width, height);
        case FrameFormat::RGBA32:
            return ConvertBGR24ToRGBA32(src, dst, width, height);
        case FrameFormat::BGRA32:
            return ConvertBGR24ToBGRA32(src, dst, width, height);
        case FrameFormat::I420:
            return ConvertBGR24ToI420(src, dst, width, height);
        default:
            return false;
    }
}

// Simple format conversions - RGB/BGR channel swapping
bool PixelFormatConverter::ConvertBGRA32ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        // BGRA -> RGBA: swap B and R channels
        dst[i * 4 + 0] = src[i * 4 + 2]; // R = B
        dst[i * 4 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 4 + 2] = src[i * 4 + 0]; // B = R
        dst[i * 4 + 3] = src[i * 4 + 3]; // A = A
    }
    return true;
}

bool PixelFormatConverter::ConvertRGBA32ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        // RGBA -> BGRA: swap R and B channels
        dst[i * 4 + 0] = src[i * 4 + 2]; // B = B
        dst[i * 4 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 4 + 2] = src[i * 4 + 0]; // R = R
        dst[i * 4 + 3] = src[i * 4 + 3]; // A = A
    }
    return true;
}

bool PixelFormatConverter::ConvertRGB24ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        // RGB -> BGR: swap R and B channels
        dst[i * 3 + 0] = src[i * 3 + 2]; // B = R
        dst[i * 3 + 1] = src[i * 3 + 1]; // G = G
        dst[i * 3 + 2] = src[i * 3 + 0]; // R = B
    }
    return true;
}

bool PixelFormatConverter::ConvertBGR24ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    return ConvertRGB24ToBGR24(src, dst, width, height); // Same operation
}

// 24-bit to 32-bit conversions (add alpha channel)
bool PixelFormatConverter::ConvertRGB24ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 3 + 0]; // R
        dst[i * 4 + 1] = src[i * 3 + 1]; // G
        dst[i * 4 + 2] = src[i * 3 + 2]; // B
        dst[i * 4 + 3] = 255;            // A = fully opaque
    }
    return true;
}

bool PixelFormatConverter::ConvertRGB24ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 3 + 2]; // B = R
        dst[i * 4 + 1] = src[i * 3 + 1]; // G = G
        dst[i * 4 + 2] = src[i * 3 + 0]; // R = B
        dst[i * 4 + 3] = 255;            // A = fully opaque
    }
    return true;
}

// 32-bit to 24-bit conversions (remove alpha channel)
bool PixelFormatConverter::ConvertBGRA32ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 3 + 0] = src[i * 4 + 2]; // R = B
        dst[i * 3 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 3 + 2] = src[i * 4 + 0]; // B = R
    }
    return true;
}

bool PixelFormatConverter::ConvertBGRA32ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 3 + 0] = src[i * 4 + 0]; // B = B
        dst[i * 3 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 3 + 2] = src[i * 4 + 2]; // R = R
    }
    return true;
}

// RGB to I420 conversions (simplified, for complex YUV you might want FFmpeg)
bool PixelFormatConverter::ConvertBGRA32ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    // Simplified RGB to YUV conversion using ITU-R BT.601 coefficients
    uint8_t *y_plane = dst;
    uint8_t *u_plane = dst + (width * height);
    uint8_t *v_plane = dst + (width * height) + (width * height / 4);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t src_idx = (y * width + x) * 4;
            uint8_t b = src[src_idx + 0];
            uint8_t g = src[src_idx + 1];
            uint8_t r = src[src_idx + 2];

            // Convert to YUV using standard coefficients
            int Y = (77 * r + 150 * g + 29 * b) >> 8;
            int U = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
            int V = ((128 * r - 107 * g - 21 * b) >> 8) + 128;

            // Clamp values
            Y = std::clamp(Y, 0, 255);
            U = std::clamp(U, 0, 255);
            V = std::clamp(V, 0, 255);

            y_plane[y * width + x] = static_cast<uint8_t>(Y);

            // Subsample U and V (4:2:0)
            if ((y % 2 == 0) && (x % 2 == 0)) {
                size_t uv_idx = (y / 2) * (width / 2) + (x / 2);
                u_plane[uv_idx] = static_cast<uint8_t>(U);
                v_plane[uv_idx] = static_cast<uint8_t>(V);
            }
        }
    }

    return true;
}

// Other format conversion implementations follow similar patterns...
bool PixelFormatConverter::ConvertRGBA32ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 3 + 0] = src[i * 4 + 0]; // R = R
        dst[i * 3 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 3 + 2] = src[i * 4 + 2]; // B = B
    }
    return true;
}

bool PixelFormatConverter::ConvertRGBA32ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 3 + 0] = src[i * 4 + 2]; // B = B
        dst[i * 3 + 1] = src[i * 4 + 1]; // G = G
        dst[i * 3 + 2] = src[i * 4 + 0]; // R = R
    }
    return true;
}

bool PixelFormatConverter::ConvertRGBA32ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    // Similar to BGRA32ToI420 but with different channel order
    uint8_t *y_plane = dst;
    uint8_t *u_plane = dst + (width * height);
    uint8_t *v_plane = dst + (width * height) + (width * height / 4);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t src_idx = (y * width + x) * 4;
            uint8_t r = src[src_idx + 0];
            uint8_t g = src[src_idx + 1];
            uint8_t b = src[src_idx + 2];

            // Convert to YUV
            int Y = (77 * r + 150 * g + 29 * b) >> 8;
            int U = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
            int V = ((128 * r - 107 * g - 21 * b) >> 8) + 128;

            Y = std::clamp(Y, 0, 255);
            U = std::clamp(U, 0, 255);
            V = std::clamp(V, 0, 255);

            y_plane[y * width + x] = static_cast<uint8_t>(Y);

            if ((y % 2 == 0) && (x % 2 == 0)) {
                size_t uv_idx = (y / 2) * (width / 2) + (x / 2);
                u_plane[uv_idx] = static_cast<uint8_t>(U);
                v_plane[uv_idx] = static_cast<uint8_t>(V);
            }
        }
    }

    return true;
}

bool PixelFormatConverter::ConvertBGR24ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 3 + 2]; // R = R
        dst[i * 4 + 1] = src[i * 3 + 1]; // G = G
        dst[i * 4 + 2] = src[i * 3 + 0]; // B = B
        dst[i * 4 + 3] = 255;            // A = fully opaque
    }
    return true;
}

bool PixelFormatConverter::ConvertBGR24ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    size_t pixel_count = width * height;
    for (size_t i = 0; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 3 + 0]; // B = B
        dst[i * 4 + 1] = src[i * 3 + 1]; // G = G
        dst[i * 4 + 2] = src[i * 3 + 2]; // R = R
        dst[i * 4 + 3] = 255;            // A = fully opaque
    }
    return true;
}

bool PixelFormatConverter::ConvertRGB24ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    uint8_t *y_plane = dst;
    uint8_t *u_plane = dst + (width * height);
    uint8_t *v_plane = dst + (width * height) + (width * height / 4);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t src_idx = (y * width + x) * 3;
            uint8_t r = src[src_idx + 0];
            uint8_t g = src[src_idx + 1];
            uint8_t b = src[src_idx + 2];

            // Convert to YUV
            int Y = (77 * r + 150 * g + 29 * b) >> 8;
            int U = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
            int V = ((128 * r - 107 * g - 21 * b) >> 8) + 128;

            Y = std::clamp(Y, 0, 255);
            U = std::clamp(U, 0, 255);
            V = std::clamp(V, 0, 255);

            y_plane[y * width + x] = static_cast<uint8_t>(Y);

            if ((y % 2 == 0) && (x % 2 == 0)) {
                size_t uv_idx = (y / 2) * (width / 2) + (x / 2);
                u_plane[uv_idx] = static_cast<uint8_t>(U);
                v_plane[uv_idx] = static_cast<uint8_t>(V);
            }
        }
    }

    return true;
}

bool PixelFormatConverter::ConvertBGR24ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height)
{
    uint8_t *y_plane = dst;
    uint8_t *u_plane = dst + (width * height);
    uint8_t *v_plane = dst + (width * height) + (width * height / 4);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t src_idx = (y * width + x) * 3;
            uint8_t b = src[src_idx + 0];
            uint8_t g = src[src_idx + 1];
            uint8_t r = src[src_idx + 2];

            // Convert to YUV
            int Y = (77 * r + 150 * g + 29 * b) >> 8;
            int U = ((-43 * r - 85 * g + 128 * b) >> 8) + 128;
            int V = ((128 * r - 107 * g - 21 * b) >> 8) + 128;

            Y = std::clamp(Y, 0, 255);
            U = std::clamp(U, 0, 255);
            V = std::clamp(V, 0, 255);

            y_plane[y * width + x] = static_cast<uint8_t>(Y);

            if ((y % 2 == 0) && (x % 2 == 0)) {
                size_t uv_idx = (y / 2) * (width / 2) + (x / 2);
                u_plane[uv_idx] = static_cast<uint8_t>(U);
                v_plane[uv_idx] = static_cast<uint8_t>(V);
            }
        }
    }

    return true;
}

size_t PixelFormatConverter::CalculateOutputFrameSize(uint32_t width, uint32_t height, FrameFormat format)
{
    switch (format) {
        case FrameFormat::RGB24:
        case FrameFormat::BGR24:
            return width * height * 3;

        case FrameFormat::RGBA32:
        case FrameFormat::BGRA32:
            return width * height * 4;

        case FrameFormat::I420:
            // Y plane + U plane (1/4) + V plane (1/4) = 1.5 * width * height
            return width * height + (width * height / 2);

        default:
            return 0;
    }
}

bool PixelFormatConverter::IsFormatSupported(FrameFormat format)
{
    switch (format) {
        case FrameFormat::RGB24:
        case FrameFormat::BGR24:
        case FrameFormat::RGBA32:
        case FrameFormat::BGRA32:
        case FrameFormat::I420:
            return true;
        default:
            return false;
    }
}

} // namespace lmshao::remotedesk