/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include "pipeline_interfaces.h"

#include <algorithm>
#include <cinttypes>
#include <mutex>

#include "../log/remote_desk_log.h"

namespace lmshao::remotedesk {
void ISource::AddSink(std::shared_ptr<ISink> sink)
{
    if (!sink) {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(sinks_mutex_);
    auto it = std::find(sinks_.begin(), sinks_.end(), sink);
    if (it == sinks_.end()) {
        LOG_DEBUG("Add sink %" PRIx64 " to source %" PRIx64, sink->GetId(), GetId());
        sinks_.push_back(sink);
    }
}

void ISource::RemoveSink(std::shared_ptr<ISink> sink)
{
    if (!sink) {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(sinks_mutex_);
    auto it = std::find(sinks_.begin(), sinks_.end(), sink);
    if (it != sinks_.end()) {
        LOG_DEBUG("Remove sink %" PRIx64 " from source %" PRIx64, sink->GetId(), GetId());
        sinks_.erase(it);
    }
}

void ISource::ClearSinks()
{
    std::unique_lock<std::shared_mutex> lock(sinks_mutex_);
    size_t count = sinks_.size();
    sinks_.clear();
    LOG_DEBUG("Clear %zu sinks from source %" PRIx64, count, GetId());
}

size_t ISource::GetSinkCount() const
{
    std::shared_lock<std::shared_mutex> lock(sinks_mutex_);
    return sinks_.size();
}

bool ISource::HasSinks() const
{
    std::shared_lock<std::shared_mutex> lock(sinks_mutex_);
    return !sinks_.empty();
}

void ISource::DeliverFrame(std::shared_ptr<Frame> frame)
{
    if (!frame || !frame->IsValid()) {
        return;
    }

    std::shared_lock<std::shared_mutex> lock(sinks_mutex_);
    for (auto &sink : sinks_) {
        if (sink) {
            sink->OnFrame(frame);
        }
    }
}

} // namespace lmshao::remotedesk
