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

#include "src/discovery/discovery_service.h"

using namespace lmshao::remotedesk;

class TestListener : public DiscoveryListener {
public:
    void OnFound(const DiscoveryInfo &info) override
    {
        std::cout << "Found device: " << info.type << ", ID: " << info.id << ", IP: " << info.ip
                  << ", Port: " << info.port << ", Version: " << info.version << std::endl;
    }
};

int main()
{
    std::cout << "Starting discovery service test..." << std::endl;

    DiscoveryService service("remote-desk", 9001, "1.0.0");
    auto listener = std::make_shared<TestListener>();
    service.SetListener(listener);

    service.Start();

    // Run for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));

    service.Stop();

    std::cout << "Discovery service test completed." << std::endl;
    return 0;
}
