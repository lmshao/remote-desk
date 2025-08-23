/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "discovery_service.h"

using namespace lmshao::remotedesk;

class MyDiscoveryListener : public DiscoveryListener {
private:
    std::string name_;

public:
    MyDiscoveryListener(const std::string &name) : name_(name) {}

    void OnFound(const DiscoveryInfo &info) override
    {
        std::cout << "[" << name_ << "] Found service: type=" << info.type << ", id=" << info.id << ", ip=" << info.ip
                  << ", port=" << info.port << ", version=" << info.version << std::endl;
    }
};

int main(int argc, char *argv[])
{
    printf("Built at %s on %s.\n", __TIME__, __DATE__);

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <service_name> <port>" << std::endl;
        std::cout << "Example: " << argv[0] << " service1 12345" << std::endl;
        return -1;
    }

    std::string service_name = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    auto listener = std::make_shared<MyDiscoveryListener>(service_name);
    DiscoveryService discovery("remote-desk", port, "1.0.0");
    discovery.SetListener(listener);
    discovery.Start();

    std::cout << "Service '" << service_name << "' discovery started on port " << port << ". Press Ctrl+C to exit..."
              << std::endl;

    // Keep running and periodically show status
    int count = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        count++;
        std::cout << "[" << service_name << "] Running for " << count * 5 << " seconds..." << std::endl;
    }

    discovery.Stop();
    return 0;
}
