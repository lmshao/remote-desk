/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_MEDIA_PROCESSOR_H
#define LMSHAO_REMOTE_DESK_MEDIA_PROCESSOR_H

#include <mutex>

#include "pipeline_interfaces.h"

namespace lmshao::remotedesk {

//==============================================================================
// Media Processor Base Class (Both Source and Sink)
//==============================================================================
class MediaProcessor : public ISource, public ISink {
public:
    MediaProcessor() = default;
    ~MediaProcessor() override = default;

    // Lifecycle management - Processors are passive and data-driven
    // They only need initialization, no active start/stop required
    virtual bool Initialize() = 0;

    // Optional: Some processors might need cleanup (e.g., releasing resources)
    virtual void Cleanup() {}

    // Status query - indicates if processor is ready to process frames
    virtual bool IsReady() const { return true; }

    // Legacy interface for backward compatibility with Pipeline
    // These are no-ops for pure data-driven processors
    virtual bool Start() { return true; }                // Always succeed - processor is passive
    virtual void Stop() {}                               // No-op - processor is passive
    virtual bool IsRunning() const { return IsReady(); } // Delegate to IsReady()

    // INode implementation
    uint64_t GetId() const override { return reinterpret_cast<uint64_t>(this); }

    // ISink implementation (input connector)
    // Pure virtual - derived classes must implement their processing logic
    void OnFrame(std::shared_ptr<Frame> frame) override = 0;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_MEDIA_PROCESSOR_H
