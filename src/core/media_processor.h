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

    // Lifecycle management
    virtual bool Initialize() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;

    // INode implementation
    uint64_t GetId() const override { return reinterpret_cast<uint64_t>(this); }

    // ISink implementation (input connector)
    // Pure virtual - derived classes must implement their processing logic
    void OnFrame(std::shared_ptr<Frame> frame) override = 0;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_MEDIA_PROCESSOR_H
