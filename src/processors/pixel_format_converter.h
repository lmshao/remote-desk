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
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
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

private:
    PixelFormatConverterConfig config_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_PIXEL_FORMAT_CONVERTER_H