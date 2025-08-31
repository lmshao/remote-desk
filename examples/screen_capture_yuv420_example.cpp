/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include <signal.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "../src/capturer/screen/screen_capture_config.h"
#include "../src/capturer/screen/screen_capturer.h"
#include "../src/core/media_sink.h"
#include "../src/core/pipeline.h"
#include "../src/processors/pixel_format_converter.h"
#include "../src/processors/video_scaler.h"

using namespace lmshao::remotedesk;

volatile bool g_running = true;
volatile bool g_interrupted = false;

void signal_handler(int signal)
{
    printf("\nReceived signal %d (Ctrl-C), stopping recording...\n", signal);
    g_running = false;
    g_interrupted = true;
}

/**
 * @brief YUV420 Frame Recorder - saves processed frames to files
 */
class YUV420FrameRecorder : public MediaSink {
public:
    YUV420FrameRecorder(const std::string &output_prefix)
        : output_prefix_(output_prefix), frame_count_(0), total_bytes_(0)
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    // MediaSink interface implementation
    bool Initialize() override { return true; }
    bool Start() override
    {
        running_ = true;
        return true;
    }
    void Stop() override { running_ = false; }
    bool IsRunning() const override { return running_; }

    void OnFrame(std::shared_ptr<Frame> frame) override
    {
        if (!running_)
            return;

        frame_count_++;
        total_bytes_ += frame->Size(); // Use Size() method from DataBuffer

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

        // Display frame information
        const char *format_name = GetFormatName(frame->format);

        printf("\rFrame %d (%dx%d) Format:%s - %llds elapsed, %zu MB", frame_count_, frame->width(), frame->height(),
               format_name, elapsed, total_bytes_ / (1024 * 1024));
        fflush(stdout);

        // Save frame to file
        SaveFrameToFile(frame);
    }

    uint64_t GetId() const override { return reinterpret_cast<uint64_t>(this); }

    void PrintSummary()
    {
        printf("\n\n=== Pipeline Recording Summary ===\n");
        printf("Total frames processed: %d\n", frame_count_);
        printf("Total data size: %.2f MB\n", static_cast<double>(total_bytes_) / (1024 * 1024));
        printf("Output files:\n");
        printf("  - Y4M video file: %s.y4m (playable with ffplay/vlc)\n", output_prefix_.c_str());
        printf("  - Raw YUV420 frames: %s_frame_*.yuv\n", output_prefix_.c_str());
        printf("\nPipeline processing chain:\n");
        printf("  Screen Capture -> Video Scaler (1920x1080) -> Pixel Format Converter (YUV420) -> Recorder\n");
        printf("\nTo play the video: ffplay %s.y4m\n", output_prefix_.c_str());
        printf(
            "To play raw YUV420: ffplay -f rawvideo -pixel_format yuv420p -video_size 1920x1080 %s_frame_000001.yuv\n",
            output_prefix_.c_str());

        if (g_interrupted) {
            printf("Recording was interrupted by user (Ctrl-C)\n");
        } else {
            printf("Recording completed successfully (timeout)\n");
        }
    }

private:
    const char *GetFormatName(FrameFormat format)
    {
        switch (format) {
            case FrameFormat::BGRA32:
                return "BGRA32";
            case FrameFormat::RGBA32:
                return "RGBA32";
            case FrameFormat::RGB24:
                return "RGB24";
            case FrameFormat::BGR24:
                return "BGR24";
            case FrameFormat::I420:
                return "I420(YUV420)";
            default:
                return "UNKNOWN";
        }
    }

    void SaveFrameToFile(std::shared_ptr<Frame> frame)
    {
        // Save as Y4M format for video playback
        SaveAsY4M(frame);

        // Save raw YUV420 data for analysis (every 30th frame)
        if (frame_count_ % 30 == 1) {
            SaveAsRawYUV420(frame);
        }
    }

    void SaveAsY4M(std::shared_ptr<Frame> frame)
    {
        static bool y4m_header_written = false;
        std::string y4m_filename = output_prefix_ + ".y4m";

        std::ofstream file(y4m_filename, std::ios::binary | (y4m_header_written ? std::ios::app : std::ios::trunc));
        if (!file.is_open()) {
            printf("\nFailed to open Y4M file %s for writing\n", y4m_filename.c_str());
            return;
        }

        // Write Y4M header only once
        if (!y4m_header_written) {
            std::string header = "YUV4MPEG2 W" + std::to_string(frame->width()) + " H" +
                                 std::to_string(frame->height()) + " F30:1 Ip A1:1 C420jpeg\n";
            file.write(header.c_str(), header.length());
            y4m_header_written = true;
        }

        // Write frame header
        file.write("FRAME\n", 6);

        // Write YUV420 data directly (already in correct format)
        if (frame->format == FrameFormat::I420) {
            file.write(reinterpret_cast<const char *>(frame->data()), frame->Size());
        } else {
            printf("\nWarning: Expected I420 format but got %s\n", GetFormatName(frame->format));
        }

        file.close();
    }

    void SaveAsRawYUV420(std::shared_ptr<Frame> frame)
    {
        std::stringstream ss;
        ss << output_prefix_ << "_frame_" << std::setfill('0') << std::setw(6) << frame_count_ << ".yuv";
        std::string filename = ss.str();

        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char *>(frame->data()), frame->Size());
            file.close();
        } else {
            printf("\nFailed to open raw YUV file %s for writing\n", filename.c_str());
        }
    }

    std::string output_prefix_;
    int frame_count_;
    size_t total_bytes_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<bool> running_{false};
};

int main(int argc, char *argv[])
{
    printf("Starting screen capture to YUV420 pipeline example...\n");
    printf("This demo uses Pipeline class to manage the complete processing chain\n\n");

    try {
        // Create pipeline
        Pipeline pipeline;

        // Create recorder (sink)
        auto recorder = std::make_shared<YUV420FrameRecorder>("screen_yuv420_pipeline");

        // Create screen capturer (source)
        ScreenCaptureConfig capture_config;
        capture_config.capture_cursor = true;
        capture_config.frame_rate = 30;
        capture_config.pixel_format = "BGRA";

        auto capturer = std::make_shared<ScreenCapturer>(capture_config, ScreenCaptureEngineFactory::Technology::Auto);

        // Create video scaler processor
        VideoScalerConfig scaler_config;
        scaler_config.target_width = 1920;  // Force different resolution
        scaler_config.target_height = 1080; // Force different resolution
        scaler_config.algorithm = ScalingAlgorithm::BILINEAR;
        scaler_config.maintain_aspect_ratio = false; // Force exact size

        auto scaler = std::make_shared<VideoScaler>(scaler_config);

        // Create pixel format converter processor
        PixelFormatConverterConfig converter_config;
        converter_config.input_format = FrameFormat::BGRA32;
        converter_config.output_format = FrameFormat::I420;
        converter_config.enable_threading = true;

        auto converter = std::make_shared<PixelFormatConverter>(converter_config);

        printf("Created processing components:\n");
        printf("  Source: %s Screen Capturer\n", capturer->GetTechnologyName().c_str());
        printf("  Processor 1: Video Scaler (-> %ux%u, force exact size)\n", scaler_config.target_width,
               scaler_config.target_height);
        printf("  Processor 2: Pixel Format Converter (BGRA32 -> I420/YUV420)\n");
        printf("  Sink: YUV420 Frame Recorder\n\n");

        // Setup pipeline: Source -> Processors -> Sink
        pipeline.SetSource(capturer);
        pipeline.AddProcessor(scaler);
        pipeline.AddProcessor(converter);
        pipeline.SetSink(recorder);

        printf("Pipeline configuration: %s\n", pipeline.GetPipelineInfo().c_str());
        printf("Total components: %zu\n\n", pipeline.GetComponentCount());

        // Link all components automatically
        if (!pipeline.LinkAll()) {
            printf("Failed to link pipeline components\n");
            return -1;
        }
        printf("Pipeline linked successfully\n");

        // Initialize all components
        printf("Initializing components...\n");
        if (!capturer->Initialize()) {
            printf("Failed to initialize screen capturer\n");
            return -1;
        }

        if (!scaler->Initialize()) {
            printf("Failed to initialize video scaler\n");
            return -1;
        }

        if (!converter->Initialize()) {
            printf("Failed to initialize pixel format converter\n");
            return -1;
        }

        if (!recorder->Initialize()) {
            printf("Failed to initialize recorder\n");
            return -1;
        }

        // Start pipeline
        printf("Starting pipeline...\n");
        if (!pipeline.Start()) {
            printf("Failed to start pipeline\n");
            return -1;
        }

        printf("Pipeline started successfully, processing frames...\n");

        // Set up signal handler for graceful shutdown
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        printf("Press Ctrl-C to stop recording...\n");

        // Wait for frames to be processed (60 seconds or until interrupted)
        auto start = std::chrono::steady_clock::now();
        int signal_check_count = 0;

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Check if we were interrupted by signal
            if (g_interrupted) {
                printf("Interrupted by user signal\n");
                break;
            }

            // Force exit if signal received multiple times
            if (!g_running) {
                signal_check_count++;
                if (signal_check_count > 10) {
                    printf("Force exit due to repeated signals\n");
                    std::exit(1);
                }
            }

            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= 60) {
                printf("\n60 seconds elapsed, stopping pipeline...\n");
                break;
            }
        }

        // Print processing summary
        recorder->PrintSummary();

        // Stop pipeline
        pipeline.Stop();
        printf("Pipeline stopped successfully\n");

        printf("Screen capture to YUV420 pipeline example completed successfully\n");

    } catch (const std::exception &e) {
        printf("Exception occurred: %s\n", e.what());
        return -1;
    }

    return 0;
}