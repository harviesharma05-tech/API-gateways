#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "DynamicArray.h"
#include "RequestQueue.h"
#include "HashRing.h"
#include "HashTable.h"
#include "Worker.h"
#include <thread>
#include <mutex>
#include <iostream>

// ThreadPool
// OS concept: fixed pool of threads pulling from a shared queue
//             instead of spawning a new thread per request.
class ThreadPool {
private:
    DynamicArray<std::thread> threads;
    RequestQueue& requestQueue;
    ConsistentHashRing& ring;
    HashTable<Worker*>& workers; // worker name -> raw pointer, O(1) lookup
    std::mutex coutMutex;

    void workerLoop() {
        ApiRequest req;
        while (requestQueue.pop(req)) {
            std::string workerName = ring.getWorker(req.clientId);

            Worker** foundWorker = workers.get(workerName);
            if (foundWorker != nullptr && (*foundWorker)->isHealthy()) {
                Worker* w = *foundWorker;
                w->incrementLoad();
                w->handleRequest(req.path);
                w->decrementLoad();
            } else {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[Gateway] No healthy worker for request "
                          << req.id << " -- dropped/retry\n";
            }
        }
    }

public:
    ThreadPool(int numThreads,
               RequestQueue& q,
               ConsistentHashRing& hashRing,
               HashTable<Worker*>& workerTable)
        : requestQueue(q), ring(hashRing), workers(workerTable) {
        for (int i = 0; i < numThreads; ++i) {
            threads.push_back(std::thread(&ThreadPool::workerLoop, this));
        }
    }

    ~ThreadPool() {
        for (int i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) threads[i].join();
        }
    }
};

#endif
