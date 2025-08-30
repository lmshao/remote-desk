/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_VIDEO_ENCODER_H
#define LMSHAO_REMOTE_DESK_VIDEO_ENCODER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "../core/media_processor.h"

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace lmshao::remotedesk {

/**
 * @brief Video encoder configuration
 */
struct VideoEncoderConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t fps = 30;
    uint32_t bitrate = 2000000;      // 2Mbps
    uint32_t keyframe_interval = 30; // Keyframe every 30 frames
    FrameFormat input_format = FrameFormat::BGRA32;
    FrameFormat output_format = FrameFormat::H264;
};

/**
 * @brief Video encoder - encodes raw video frames to H264
 * Inherits from MediaProcessor as a processing node in Pipeline
 */
class VideoEncoder : public MediaProcessor {
public:
    explicit VideoEncoder(const VideoEncoderConfig &config = {});
    ~VideoEncoder() override;

    // MediaProcessor interface implementation
    bool Initialize() override;
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    void OnFrame(std::shared_ptr<Frame> frame) override;

    /**
     * @brief Update configuration (dynamic adjustment)
     */
    bool UpdateConfig(const VideoEncoderConfig &config);

    /**
     * @brief Get current configuration
     */
    const VideoEncoderConfig &GetConfig() const { return config_; }

    /**
     * @brief Dynamically adjust bitrate
     */
    bool SetBitrate(uint32_t bitrate);

    /**
     * @brief Force generate keyframe
     */
    void ForceKeyFrame();

    /**
     * @brief Flush encoder buffer
     */
    void Flush();

    /**
     * @brief Get encoding statistics
     */
    struct EncodeStats {
        uint64_t frames_received = 0;
        uint64_t frames_encoded = 0;
        uint64_t frames_dropped = 0;
        uint32_t current_fps = 0;
        uint32_t current_bitrate = 0;
        std::chrono::milliseconds avg_encode_time{0};
        uint64_t total_bytes_encoded = 0;
    };
    EncodeStats GetStats() const;

private:
    /**
     * @brief Encoding thread function
     */
    void EncodeThreadFunc();

    /**
     * @brief Initialize FFmpeg encoder
     */
    bool InitializeFFmpeg();

    /**
     * @brief Cleanup FFmpeg resources
     */
    void CleanupFFmpeg();

    /**
     * @brief Convert pixel format
     */
    bool ConvertPixelFormat(std::shared_ptr<Frame> input_frame, AVFrame *av_frame);

    /**
     * @brief Process encoded packet
     */
    void ProcessEncodedPacket(AVPacket *packet);

    /**
     * @brief Update statistics
     */
    void UpdateStats(size_t encoded_size);

private:
    VideoEncoderConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> force_keyframe_{false};

    // FFmpeg related
    const AVCodec *codec_ = nullptr;
    AVCodecContext *codec_ctx_ = nullptr;
    AVFrame *frame_ = nullptr;
    AVPacket *packet_ = nullptr;
    SwsContext *sws_ctx_ = nullptr;

    // Encoding queue
    std::queue<std::shared_ptr<Frame>> encode_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Encoding thread
    std::thread encode_thread_;

    // Status management
    std::mutex encode_mutex_;

    // Statistics
    mutable std::mutex stats_mutex_;
    EncodeStats stats_;
    std::chrono::steady_clock::time_point last_stats_time_;

    // Frame count
    uint64_t frame_count_ = 0;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_VIDEO_ENCODER_H