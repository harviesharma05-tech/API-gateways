#ifndef WORKER_H
#define WORKER_H

#include <string>
#include <atomic>
#include <iostream>

// Base Worker class
// OOP concepts: Inheritance, Polymorphism (virtual functions)
class Worker {
protected:
    std::string name;
    std::atomic<int> activeConnections;
    std::atomic<bool> healthy;

public:
    Worker(const std::string& workerName)
        : name(workerName), activeConnections(0), healthy(true) {}

    virtual ~Worker() = default;

    std::string getName() const { return name; }
    int getLoad() const { return activeConnections.load(); }
    bool isHealthy() const { return healthy.load(); }
    void setHealthy(bool status) { healthy.store(status); }

    void incrementLoad() { activeConnections++; }
    void decrementLoad() { if (activeConnections > 0) activeConnections--; }

    // Virtual method overridden differently per worker type (polymorphism)
    virtual void handleRequest(const std::string& requestPath) {
        std::cout << "[" << name << "] handling request " << requestPath << "\n";
    }
};

// Derived class: specialized for read-heavy endpoints
class ReadWorker : public Worker {
public:
    ReadWorker(const std::string& workerName) : Worker(workerName) {}

    void handleRequest(const std::string& requestPath) override {
        std::cout << "[ReadWorker:" << name << "] serving cached/read request "
                  << requestPath << "\n";
    }
};

// Derived class: specialized for write-heavy endpoints
class WriteWorker : public Worker {
public:
    WriteWorker(const std::string& workerName) : Worker(workerName) {}

    void handleRequest(const std::string& requestPath) override {
        std::cout << "[WriteWorker:" << name << "] persisting write request "
                  << requestPath << "\n";
    }
};

#endif
