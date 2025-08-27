/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_MEDIA_SOURCE_H
#define LMSHAO_REMOTE_DESK_MEDIA_SOURCE_H

#include "pipeline_interfaces.h"

namespace lmshao::remotedesk {

//==============================================================================
// Media Source Base Class
//==============================================================================
class MediaSource : public ISource {
public:
    MediaSource() = default;
    ~MediaSource() override = default;

    // Lifecycle management
    virtual bool Initialize() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;

    // INode implementation
    uint64_t GetId() const override { return reinterpret_cast<uint64_t>(this); }
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_MEDIA_SOURCE_H
