#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <string>
#include <mutex>
#include <condition_variable>

// A single API request travelling through the gateway.
struct ApiRequest {
    int id;
    std::string path;
    std::string clientId;
};

// RequestQueue
// DSA concept: Circular Queue implemented on a fixed-size array
//              (no std::queue) — classic front/rear with wraparound.
// OS concept: mutex + condition_variable for producer-consumer sync
//             between the gateway thread and the worker thread pool.
class RequestQueue {
private:
    static const int CAPACITY = 256;
    ApiRequest buffer[CAPACITY];
    int front;
    int rear;
    int count;

    mutable std::mutex mtx;
    std::condition_variable cv;
    bool shuttingDown;

public:
    RequestQueue() : front(0), rear(0), count(0), shuttingDown(false) {}

    void push(const ApiRequest& req) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (count == CAPACITY) {
                // Queue full — drop oldest (simple backpressure policy)
                front = (front + 1) % CAPACITY;
                count--;
            }
            buffer[rear] = req;
            rear = (rear + 1) % CAPACITY;
            count++;
        }
        cv.notify_one();
    }

    // Blocks until a request is available or shutdown is signalled.
    bool pop(ApiRequest& outReq) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return count > 0 || shuttingDown; });

        if (count == 0) return false; // shutting down, nothing left

        outReq = buffer[front];
        front = (front + 1) % CAPACITY;
        count--;
        return true;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            shuttingDown = true;
        }
        cv.notify_all();
    }

    int size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return count;
    }
};

#endif
