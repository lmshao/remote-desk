/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "service_manager.h"

using namespace lmshao::remotedesk;

int main()
{
    auto mgr = ServiceManager::GetInstance();

    // Display services and start them
    auto services = mgr->GetAllServices();
    for (const auto &service : services) {
        bool started = mgr->StartService(service);
        std::cout << service << ": " << (started ? "OK" : "FAILED") << std::endl;
    }

    // Run for a while
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop all services
    mgr->StopAllServices();

    return 0;
}