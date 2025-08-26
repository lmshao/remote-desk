/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_FRAME_H
#define LMSHAO_REMOTE_DESK_FRAME_H

#include <coreutils/data_buffer.h>

#include <cstdint>

namespace lmshao::remotedesk {

using namespace lmshao::coreutils;
enum class FrameFormat : int {
    UNKNOWN = 0,

    // Video formats (100-199)
    VIDEO_BASE = 100,
    I420 = 101,
    NV12 = 102,
    RGB24 = 103,
    BGR24 = 104,
    RGBA32 = 105,
    BGRA32 = 106,
    H264 = 107,
    H265 = 108,
    VP8 = 109,
    VP9 = 110,

    // Audio formats (200-299)
    AUDIO_BASE = 200,
    PCM_S16LE = 201,
    PCM_F32LE = 202,
    AAC = 203,
    MP3 = 204,
    OPUS = 205,
    G711_PCMU = 206,
    G711_PCMA = 207,
};

inline FrameFormat GetFrameType(FrameFormat format)
{
    int value = static_cast<int>(format);
    return static_cast<FrameFormat>((value / 100) * 100);
}

struct VideoFrameInfo {
    uint16_t width = 0;
    uint16_t height = 0;
    uint32_t framerate = 0;
    bool is_keyframe = false;
};

struct AudioFrameInfo {
    uint8_t channels = 0;
    uint32_t sample_rate = 0;
    uint32_t nb_samples = 0;
    uint32_t bytes_per_sample = 0;
};

class Frame : public DataBuffer {
public:
    template <typename... Args>
    explicit Frame(Args... args) : DataBuffer(args...)
    {
    }

    int64_t timestamp = 0;
    FrameFormat format = FrameFormat::UNKNOWN;

    union {
        VideoFrameInfo video_info;
        AudioFrameInfo audio_info;
    };

    bool IsValid() const { return Data() != nullptr && Size() > 0; }
    bool IsVideo() const { return GetFrameType(format) == FrameFormat::VIDEO_BASE; }
    bool IsAudio() const { return GetFrameType(format) == FrameFormat::AUDIO_BASE; }
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_FRAME_H
