/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_PIPELINE_H
#define LMSHAO_REMOTE_DESK_PIPELINE_H

#include <memory>
#include <vector>

#include "media_processor.h"
#include "media_sink.h"
#include "media_source.h"

namespace lmshao::remotedesk {

class Pipeline {
public:
    Pipeline() = default;
    ~Pipeline() = default;

    void AddProcessor(std::shared_ptr<MediaProcessor> processor)
    {
        if (processor) {
            processors_.push_back(processor);
        }
    }

    void SetSource(std::shared_ptr<MediaSource> source) { source_ = source; }

    void SetSink(std::shared_ptr<MediaSink> sink) { sink_ = sink; }

    bool LinkAll()
    {
        if (!source_ || !sink_) {
            return false;
        }

        if (processors_.empty()) {
            // Direct connection: Source -> Sink
            source_->AddSink(sink_);
        } else {
            // Chain connection: Source -> Processors -> Sink

            // Link Source -> First Processor
            source_->AddSink(processors_.front());

            // Link Processors
            for (size_t i = 0; i < processors_.size() - 1; ++i) {
                processors_[i]->AddSink(processors_[i + 1]);
            }

            // Link Last Processor -> Sink
            processors_.back()->AddSink(sink_);
        }

        return true;
    }

    bool Start()
    {
        // Start all processors first
        for (auto &processor : processors_) {
            if (processor && !processor->Start()) {
                return false;
            }
        }

        // Start sink
        if (sink_ && !sink_->Start()) {
            return false;
        }

        // Start source last
        return source_ && source_->Start();
    }

    void Stop()
    {
        // Stop source first
        if (source_) {
            source_->Stop();
        }

        // Stop all processors
        for (auto &processor : processors_) {
            if (processor) {
                processor->Stop();
            }
        }

        // Stop sink last
        if (sink_) {
            sink_->Stop();
        }
    }

    bool IsConnected() const { return source_ && sink_; }

    void UnlinkAll()
    {
        if (source_) {
            source_->ClearSinks();
        }
        for (auto &processor : processors_) {
            if (processor) {
                processor->ClearSinks();
            }
        }
    }

    void Clear()
    {
        UnlinkAll();
        source_.reset();
        sink_.reset();
        processors_.clear();
    }

    size_t GetComponentCount() const
    {
        size_t count = 0;
        if (source_)
            count++;
        if (sink_)
            count++;
        count += processors_.size();
        return count;
    }

    std::string GetPipelineInfo() const
    {
        std::string info = "Pipeline: ";
        if (source_) {
            info += "Source";
        }
        if (!processors_.empty()) {
            info += " -> " + std::to_string(processors_.size()) + " Processor(s)";
        }
        if (sink_) {
            info += " -> Sink";
        }
        return info;
    }

private:
    std::shared_ptr<MediaSource> source_;
    std::vector<std::shared_ptr<MediaProcessor>> processors_;
    std::shared_ptr<MediaSink> sink_;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_PIPELINE_H
