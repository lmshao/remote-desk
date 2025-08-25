/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_MESSAGE_SERVICE_H
#define LMSHAO_REMOTE_DESK_MESSAGE_SERVICE_H

#include <atomic>

#include "../../core/service_manager.h"

namespace lmshao::remotedesk {

class MessageService : public ServiceBase {
    REGISTER_SERVICE(MessageService, "MESSAGE_SERVICE");

public:
    MessageService();
    ~MessageService() override;

    // ServiceBase interface
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;

private:
    std::atomic<bool> running_{false};
    int port_ = 8080;
};

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_MESSAGE_SERVICE_H