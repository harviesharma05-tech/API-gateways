#ifndef HASH_RING_H
#define HASH_RING_H

#include <string>
#include "DynamicArray.h"

// A single point on the ring: a hash value mapped to a worker name.
struct RingNode {
    unsigned long hashValue;
    std::string workerName;
};

// ConsistentHashRing
// DSA concepts: Hashing + Sorted Array + Binary Search
// The ring is kept as a manually sorted DynamicArray (not std::map).
// Insertion keeps it sorted; lookup uses binary search for the
// first node whose hash >= the request's hash (classic consistent hashing).
class ConsistentHashRing {
private:
    DynamicArray<RingNode> ring;
    int virtualNodesPerWorker;

    // Hand-written hash function (FNV-1a with an avalanche finalizer).
    // Plain FNV-1a alone doesn't mix bits well for short, similar-prefix
    // strings (e.g. "Worker-A#0" vs "Worker-A#1"), which clustered all
    // virtual nodes together and broke ring distribution. The finalizer
    // (murmur-style xor-shift-multiply) spreads bits across the full
    // 64-bit range so nodes land uniformly around the ring.
    unsigned long hashFunction(const std::string& key) const {
        unsigned long hash = 14695981039346656037UL; // FNV-1a 64-bit offset basis
        for (char c : key) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211UL;               // FNV-1a 64-bit prime
        }
        hash ^= (hash >> 33);
        hash *= 0xff51afd7ed558ccdUL;
        hash ^= (hash >> 33);
        hash *= 0xc4ceb9fe1a85ec53UL;
        hash ^= (hash >> 33);
        return hash;
    }

    // Binary search for the correct insertion index to keep `ring` sorted
    // by hashValue. Returns the index of the first element >= target.
    int lowerBound(unsigned long targetHash) const {
        int lo = 0, hi = ring.size(); // hi is one-past-end
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (ring[mid].hashValue < targetHash) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

public:
    explicit ConsistentHashRing(int virtualNodes = 50)
        : virtualNodesPerWorker(virtualNodes) {}

    void addWorker(const std::string& workerName) {
        for (int i = 0; i < virtualNodesPerWorker; ++i) {
            std::string virtualKey = workerName + "#" + std::to_string(i);
            unsigned long h = hashFunction(virtualKey);
            int pos = lowerBound(h);
            ring.insertAt(pos, {h, workerName});
        }
    }

    void removeWorker(const std::string& workerName) {
        // Linear scan removing all virtual nodes belonging to this worker.
        for (int i = ring.size() - 1; i >= 0; --i) {
            if (ring[i].workerName == workerName) {
                ring.removeAt(i);
            }
        }
    }

    // Find the worker responsible for a given request key — O(log n) search
    std::string getWorker(const std::string& requestKey) const {
        if (ring.isEmpty()) return "";

        unsigned long h = hashFunction(requestKey);
        int pos = lowerBound(h);

        if (pos == ring.size()) {
            pos = 0; // wrap around the ring
        }
        return ring[pos].workerName;
    }

    bool isEmpty() const { return ring.isEmpty(); }
    int nodeCount() const { return ring.size(); }
};

#endif
