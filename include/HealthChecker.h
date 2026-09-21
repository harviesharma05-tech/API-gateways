#ifndef HEALTH_CHECKER_H
#define HEALTH_CHECKER_H

#include "HashTable.h"
#include "HashRing.h"
#include "Worker.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

// HealthChecker
// OS concept: background thread doing periodic monitoring.
// Purpose: marks a worker unhealthy and pulls it out of the hash ring
//          (failover), simulating real gateway behaviour.
class HealthChecker {
private:
    HashTable<Worker*>& workers;
    ConsistentHashRing& ring;
    std::atomic<bool> running{true};
    std::thread checkerThread;
    int intervalMs;
    HashTable<bool> alreadyRemoved; // tracks removal state per worker name

    void loop() {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

            workers.forEach([this](const std::string& name, Worker* worker) {
                bool* removedFlag = alreadyRemoved.get(name);
                bool wasRemoved = (removedFlag != nullptr && *removedFlag);

                if (!worker->isHealthy() && !wasRemoved) {
                    std::cout << "[HealthChecker] " << name
                              << " is DOWN -> removing from hash ring\n";
                    ring.removeWorker(name);
                    alreadyRemoved.put(name, true);
                } else if (worker->isHealthy() && wasRemoved) {
                    std::cout << "[HealthChecker] " << name
                              << " recovered -> re-adding to hash ring\n";
                    ring.addWorker(name);
                    alreadyRemoved.put(name, false);
                }
            });
        }
    }

public:
    HealthChecker(HashTable<Worker*>& workerTable,
                  ConsistentHashRing& hashRing,
                  int checkIntervalMs = 300)
        : workers(workerTable), ring(hashRing), intervalMs(checkIntervalMs) {
        checkerThread = std::thread(&HealthChecker::loop, this);
    }

    void stop() {
        running.store(false);
        if (checkerThread.joinable()) checkerThread.join();
    }

    ~HealthChecker() {
        stop();
    }
};

#endif
