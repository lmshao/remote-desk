/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_PIPELINE_INTERFACES_H
#define LMSHAO_REMOTE_DESK_PIPELINE_INTERFACES_H

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <vector>

#include "frame.h"

namespace lmshao::remotedesk {

class INode {
public:
    virtual ~INode() = default;

    virtual uint64_t GetId() const = 0;
};

class ISink;
class ISource : public virtual INode {
public:
    virtual ~ISource() = default;

    void AddSink(std::shared_ptr<ISink> sink);
    void RemoveSink(std::shared_ptr<ISink> sink);
    void ClearSinks();

    size_t GetSinkCount() const;
    bool HasSinks() const;

protected:
    void DeliverFrame(std::shared_ptr<Frame> frame);

private:
    std::vector<std::shared_ptr<ISink>> sinks_;
    mutable std::shared_mutex sinks_mutex_;
};

class ISink : public virtual INode {
public:
    virtual ~ISink() = default;

    virtual void OnFrame(std::shared_ptr<Frame> frame) = 0;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_PIPELINE_INTERFACES_H
