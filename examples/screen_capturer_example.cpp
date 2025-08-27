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
#include "../src/core/pipeline_interfaces.h"

using namespace lmshao::remotedesk;

volatile bool g_running = true;
volatile bool g_interrupted = false;

void signal_handler(int signal)
{
    printf("\nReceived signal %d (Ctrl-C), stopping recording...\n", signal);
    g_running = false;
    g_interrupted = true;
}

class ScreenRecorder : public ISink {
public:
    ScreenRecorder(const std::string &output_prefix) : output_prefix_(output_prefix), frame_count_(0), total_bytes_(0)
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void OnFrame(std::shared_ptr<Frame> frame) override
    {
        frame_count_++;
        total_bytes_ += frame->Size();

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

        // Display frame format information
        const char *format_name = "UNKNOWN";

        if (frame->format == FrameFormat::BGRA32)
            format_name = "BGRA32";
        else if (frame->format == FrameFormat::RGBA32)
            format_name = "RGBA32";

        printf("\rFrame %d (%dx%d) Format:%s - %lds elapsed, %zu MB", frame_count_, frame->video_info.width,
               frame->video_info.height, format_name, elapsed, total_bytes_ / (1024 * 1024));
        fflush(stdout);

        // Save frame data to file
        SaveFrameToFile(frame);
    }

    uint64_t GetId() const override { return reinterpret_cast<uint64_t>(this); }

    void PrintSummary()
    {
        printf("\n\n=== Recording Summary ===\n");
        printf("Total frames captured: %d\n", frame_count_);
        printf("Total data size: %.2f MB\n", static_cast<double>(total_bytes_) / (1024 * 1024));
        printf("Output files:\n");
        printf("  - Y4M video file: %s.y4m (playable with ffplay/vlc)\n", output_prefix_.c_str());
        printf("  - Raw format frames: %s_frame_*.{bgra,rgba,raw} (direct X11 output)\n", output_prefix_.c_str());
        printf("\nTo play the video: ffplay %s.y4m\n", output_prefix_.c_str());
        printf("Raw format files contain zero-copy data directly from X11 capture\n");
        if (g_interrupted) {
            printf("Recording was interrupted by user (Ctrl-C)\n");
        } else {
            printf("Recording completed successfully (1 minute timeout)\n");
        }
    }

private:
    void SaveFrameToFile(std::shared_ptr<Frame> frame)
    {
        // Save as Y4M format for video playback compatibility
        SaveAsY4M(frame);

        // Also save raw RGB data for analysis
        SaveAsRawRGB(frame);
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
            std::string header = "YUV4MPEG2 W" + std::to_string(frame->video_info.width) + " H" +
                                 std::to_string(frame->video_info.height) + " F10:1 Ip A1:1 C444\n";
            file.write(header.c_str(), header.length());
            y4m_header_written = true;
        }

        // Write frame header
        file.write("FRAME\n", 6);

        // Convert RGB to YUV444 and write
        ConvertRGBToYUV444AndWrite(frame, file);
        file.close();
    }

    void SaveAsRawRGB(std::shared_ptr<Frame> frame)
    {
        std::stringstream ss;

        // Use appropriate file extension based on format
        const char *extension = "raw";
        if (frame->format == FrameFormat::BGRA32) {
            extension = "bgra";
        } else if (frame->format == FrameFormat::RGBA32) {
            extension = "rgba";
        }

        ss << output_prefix_ << "_frame_" << std::setfill('0') << std::setw(6) << frame_count_ << "." << extension;
        std::string filename = ss.str();

        std::ofstream file(filename, std::ios::binary);
        if (file.is_open()) {
            // Write raw format data directly (zero-copy from X11)
            file.write(reinterpret_cast<const char *>(frame->Data()), frame->Size());
            file.close();
        } else {
            printf("\nFailed to open raw format file %s for writing\n", filename.c_str());
        }
    }

    void ConvertRGBToYUV444AndWrite(std::shared_ptr<Frame> frame, std::ofstream &file)
    {
        const uint8_t *rgb_data = frame->Data();
        int width = frame->video_info.width;
        int height = frame->video_info.height;

        // Allocate YUV buffers
        std::vector<uint8_t> y_plane(width * height);
        std::vector<uint8_t> u_plane(width * height);
        std::vector<uint8_t> v_plane(width * height);

        // Convert RGB to YUV - handle both BGRA and RGBA formats
        for (int i = 0; i < width * height; i++) {
            uint8_t r, g, b;

            if (frame->format == FrameFormat::BGRA32) {
                // BGRA format: B=0, G=1, R=2, A=3
                b = rgb_data[i * 4 + 0];
                g = rgb_data[i * 4 + 1];
                r = rgb_data[i * 4 + 2];
            } else if (frame->format == FrameFormat::RGBA32) {
                // RGBA format: R=0, G=1, B=2, A=3
                r = rgb_data[i * 4 + 0];
                g = rgb_data[i * 4 + 1];
                b = rgb_data[i * 4 + 2];
            } else {
                // Fallback: assume BGRA format
                b = rgb_data[i * 4 + 0];
                g = rgb_data[i * 4 + 1];
                r = rgb_data[i * 4 + 2];
            }

            // YUV conversion (ITU-R BT.601)
            int y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
            int u = (int)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
            int v = (int)(0.500 * r - 0.419 * g - 0.081 * b + 128);

            y_plane[i] = std::clamp(y, 0, 255);
            u_plane[i] = std::clamp(u, 0, 255);
            v_plane[i] = std::clamp(v, 0, 255);
        }

        // Write YUV planes
        file.write(reinterpret_cast<const char *>(y_plane.data()), y_plane.size());
        file.write(reinterpret_cast<const char *>(u_plane.data()), u_plane.size());
        file.write(reinterpret_cast<const char *>(v_plane.data()), v_plane.size());
    }

    std::string output_prefix_;
    int frame_count_;
    size_t total_bytes_;
    std::chrono::steady_clock::time_point start_time_;
};

int main(int argc, char *argv[])
{
    printf("Starting screen capturer example...\n");

    try {
        // Create screen capture configuration
        ScreenCaptureConfig config;
        config.capture_cursor = true;
        config.frame_rate = 30;       // 5 FPS for testing
        config.pixel_format = "BGRA"; // Use BGRA format

        // Create screen capturer with configuration
        auto capturer = std::make_unique<ScreenCapturer>(config, ScreenCaptureEngineFactory::Technology::X11);

        printf("Created screen capturer using technology: %s\n", capturer->GetTechnologyName().c_str());

        // Create screen recorder
        auto screen_recorder = std::make_shared<ScreenRecorder>("screen_capturer");

        // Add screen recorder as sink
        capturer->AddSink(screen_recorder);

        // Initialize capturer
        if (!capturer->Initialize()) {
            printf("Failed to initialize screen capturer\n");
            return -1;
        }

        // Start capturing
        if (!capturer->Start()) {
            printf("Failed to start screen capturer\n");
            return -1;
        }

        printf("Screen capture started, capturing frames...\n");

        // Set up signal handler for graceful shutdown
        signal(SIGINT, signal_handler);

        // Wait for frames to be captured (60 seconds or until interrupted)
        auto start = std::chrono::steady_clock::now();
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= 60) {
                printf("\n60 seconds elapsed, stopping capture...\n");
                break;
            }
        }

        // Print recording summary
        screen_recorder->PrintSummary();

        // Stop capturing
        capturer->Stop();

        printf("Screen capture example completed successfully\n");

    } catch (const std::exception &e) {
        printf("Exception occurred: %s\n", e.what());
        return -1;
    }

    return 0;
}