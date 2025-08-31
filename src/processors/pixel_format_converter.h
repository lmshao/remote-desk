/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_PIXEL_FORMAT_CONVERTER_H
#define LMSHAO_REMOTE_DESK_PIXEL_FORMAT_CONVERTER_H

#include <atomic>
#include <mutex>

#include "../core/media_processor.h"

namespace lmshao::remotedesk {

/**
 * @brief Pixel format converter configuration
 */
struct PixelFormatConverterConfig {
    FrameFormat input_format = FrameFormat::BGRA32;
    FrameFormat output_format = FrameFormat::I420;
    bool enable_threading = true;
};

/**
 * @brief Pixel format converter - converts between different pixel formats
 * Inherits from MediaProcessor, works as a processing node in pipeline
 */
class PixelFormatConverter : public MediaProcessor {
public:
    explicit PixelFormatConverter(const PixelFormatConverterConfig &config = {});
    ~PixelFormatConverter() override;

    // Format conversion interface
    bool SetOutputFormat(FrameFormat format);
    FrameFormat GetInputFormat() const { return config_.input_format; }
    FrameFormat GetOutputFormat() const { return config_.output_format; }

    // MediaProcessor interface implementation
    bool Initialize() override;
    void OnFrame(std::shared_ptr<Frame> frame) override;

private:
    /**
     * @brief Convert pixel format using software conversion
     */
    std::shared_ptr<Frame> ConvertFrame(std::shared_ptr<Frame> input_frame);

    /**
     * @brief Calculate output frame size
     */
    size_t CalculateOutputFrameSize(uint32_t width, uint32_t height, FrameFormat format);

    /**
     * @brief Check if format is supported by software converter
     */
    bool IsFormatSupported(FrameFormat format);

    /**
     * @brief Format-specific conversion methods
     */
    bool ConvertFromBGRA32(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output);
    bool ConvertFromRGBA32(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output);
    bool ConvertFromRGB24(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output);
    bool ConvertFromBGR24(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output);

    /**
     * @brief Low-level conversion functions
     */
    bool ConvertBGRA32ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGBA32ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGB24ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertBGR24ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);

    // 24-bit to 32-bit conversions
    bool ConvertRGB24ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGB24ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertBGR24ToRGBA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertBGR24ToBGRA32(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);

    // 32-bit to 24-bit conversions
    bool ConvertBGRA32ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertBGRA32ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGBA32ToRGB24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGBA32ToBGR24(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);

    // RGB to YUV conversions (simplified)
    bool ConvertBGRA32ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGBA32ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertRGB24ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);
    bool ConvertBGR24ToI420(const uint8_t *src, uint8_t *dst, uint32_t width, uint32_t height);

private:
    PixelFormatConverterConfig config_;
    mutable std::mutex mutex_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_PIXEL_FORMAT_CONVERTER_H