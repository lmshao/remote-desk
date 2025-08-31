/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_VIDEO_SCALER_H
#define LMSHAO_REMOTE_DESK_VIDEO_SCALER_H

#include <atomic>
#include <mutex>

#include "../core/media_processor.h"

namespace lmshao::remotedesk {

/**
 * @brief Video scaling algorithms
 */
enum class ScalingAlgorithm {
    BILINEAR = 0, // Fast bilinear interpolation
    BICUBIC = 1,  // High quality bicubic interpolation
    LANCZOS = 2,  // Highest quality Lanczos algorithm
    NEAREST = 3   // Fastest nearest neighbor (pixelated)
};

/**
 * @brief Video scaler configuration
 */
struct VideoScalerConfig {
    uint32_t target_width = 1920;
    uint32_t target_height = 1080;
    ScalingAlgorithm algorithm = ScalingAlgorithm::BILINEAR;
    bool maintain_aspect_ratio = true;
    bool enable_threading = true;
};

/**
 * @brief Video scaler - scales video frames to target resolution
 * Inherits from MediaProcessor, works as a processing node in pipeline
 */
class VideoScaler : public MediaProcessor {
public:
    explicit VideoScaler(const VideoScalerConfig &config = {});
    ~VideoScaler() override;

    // Scaling interface
    bool SetTargetResolution(uint32_t width, uint32_t height);
    bool SetScalingAlgorithm(ScalingAlgorithm algorithm);

    uint32_t GetTargetWidth() const { return config_.target_width; }
    uint32_t GetTargetHeight() const { return config_.target_height; }
    ScalingAlgorithm GetScalingAlgorithm() const { return config_.algorithm; }

    // MediaProcessor interface implementation
    bool Initialize() override;
    void OnFrame(std::shared_ptr<Frame> frame) override;

    /**
     * @brief Get scaling statistics
     */
    struct ScalingStats {
        uint64_t frames_processed = 0;
        uint64_t frames_dropped = 0;
        std::chrono::milliseconds avg_scaling_time{0};
        uint32_t input_width = 0;
        uint32_t input_height = 0;
        uint32_t output_width = 0;
        uint32_t output_height = 0;
    };
    ScalingStats GetStats() const;

private:
    /**
     * @brief Scale frame using software scaling
     */
    std::shared_ptr<Frame> ScaleFrame(std::shared_ptr<Frame> input_frame);

    /**
     * @brief Calculate target dimensions maintaining aspect ratio
     */
    std::pair<uint32_t, uint32_t> CalculateTargetDimensions(uint32_t input_width, uint32_t input_height) const;

    /**
     * @brief Check if scaling is needed
     */
    bool IsScalingNeeded(uint32_t input_width, uint32_t input_height) const;

    /**
     * @brief Update statistics
     */
    void UpdateStats(uint32_t input_width, uint32_t input_height, uint32_t output_width, uint32_t output_height,
                     std::chrono::milliseconds processing_time);

    /**
     * @brief Perform bilinear scaling for BGRA/RGBA formats
     */
    void PerformBilinearScaling(std::shared_ptr<Frame> input, std::shared_ptr<Frame> output);

private:
    VideoScalerConfig config_;
    mutable std::mutex mutex_;

    // Statistics
    mutable std::mutex stats_mutex_;
    ScalingStats stats_;
    std::chrono::steady_clock::time_point last_stats_time_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_VIDEO_SCALER_H