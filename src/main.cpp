#include <iostream>
#include <thread>
#include <chrono>
#include "HashTable.h"
#include "HashRing.h"
#include "TokenBucket.h"
#include "Worker.h"
#include "RequestQueue.h"
#include "ThreadPool.h"
#include "HealthChecker.h"

int main() {
    std::cout << "=========================================\n";
    std::cout << " Asynchronous HTTP API Gateway - Prototype\n";
    std::cout << " (Pure C++ + hand-written Data Structures)\n";
    std::cout << "=========================================\n\n";

    // ---------- 1. Create workers (OOP: inheritance + polymorphism) -------
    // Raw pointers + manual new/delete, no smart pointers.
    Worker* workerA = new ReadWorker("Worker-A");
    Worker* workerB = new WriteWorker("Worker-B");
    Worker* workerC = new ReadWorker("Worker-C");

    // ---------- 2. Hand-written hash table: name -> Worker* (O(1) lookup) -
    HashTable<Worker*> workers;
    workers.put(workerA->getName(), workerA);
    workers.put(workerB->getName(), workerB);
    workers.put(workerC->getName(), workerC);

    // ---------- 3. Hand-written consistent hash ring (sorted array + -----
    //              binary search, no std::map)
    ConsistentHashRing ring(50);
    ring.addWorker(workerA->getName());
    ring.addWorker(workerB->getName());
    ring.addWorker(workerC->getName());
    std::cout << "Hash ring initialized with " << ring.nodeCount()
              << " virtual nodes.\n\n";

    // ---------- 4. Rate limiter per client (hand-written hash table) ------
    HashTable<TokenBucket> clientBuckets;
    clientBuckets.put("client-1", TokenBucket(5, 2));  // 5 burst, 2/sec refill
    clientBuckets.put("client-7", TokenBucket(3, 1));

    // ---------- 5. Circular queue + thread pool (OS: producer-consumer) ---
    RequestQueue queue;
    ThreadPool pool(3, queue, ring, workers); // 3 worker threads

    // ---------- 6. Background health checker (OS: monitoring thread) -----
    HealthChecker healthChecker(workers, ring, 300);

    // ---------- 7. Simulate incoming API requests --------------------------
    const char* clientList[] = {"client-1", "client-7"};
    int requestId = 1;

    std::cout << "Dispatching simulated requests...\n\n";
    for (int round = 0; round < 8; ++round) {
        for (const char* client : clientList) {
            TokenBucket* bucket = clientBuckets.get(client);
            if (bucket->allowRequest()) {
                queue.push({requestId++, "/api/resource", client});
            } else {
                std::cout << "[RateLimiter] " << client
                          << " exceeded rate limit -- request rejected\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    // ---------- 8. Simulate a worker going down -> failover ---------------
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "\n[Simulation] Worker-B going down...\n";
    workerB->setHealthy(false);

    std::this_thread::sleep_for(std::chrono::milliseconds(700));

    for (int i = 0; i < 4; ++i) {
        queue.push({requestId++, "/api/resource", "client-3"}); // was routed to Worker-B
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // ---------- 9. Clean shutdown -------------------------------------------
    queue.shutdown();
    healthChecker.stop();

    // Manual cleanup — every `new` is matched with a `delete`
    delete workerA;
    delete workerB;
    delete workerC;

    std::cout << "\nGateway shutdown complete.\n";
    return 0;
}
