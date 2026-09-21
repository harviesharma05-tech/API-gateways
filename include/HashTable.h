#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <string>

// DSA concept: Hash Table with Separate Chaining
// Each bucket is a manually-built singly linked list. No std::unordered_map.
template <typename V>
struct ChainNode {
    std::string key;
    V value;
    ChainNode* next;
    ChainNode(const std::string& k, const V& v) : key(k), value(v), next(nullptr) {}
};

template <typename V>
class HashTable {
private:
    static const int TABLE_SIZE = 101; // prime size reduces clustering
    ChainNode<V>* buckets[TABLE_SIZE];

    // Simple polynomial rolling hash (djb2-style), written by hand.
    unsigned int hashFunction(const std::string& key) const {
        unsigned long hash = 5381;
        for (char c : key) {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash*33 + c
        }
        return static_cast<unsigned int>(hash % TABLE_SIZE);
    }

public:
    HashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) buckets[i] = nullptr;
    }

    ~HashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            ChainNode<V>* node = buckets[i];
            while (node) {
                ChainNode<V>* next = node->next;
                delete node;
                node = next;
            }
        }
    }

    // Insert or update — O(1) average
    void put(const std::string& key, const V& value) {
        unsigned int index = hashFunction(key);
        ChainNode<V>* node = buckets[index];
        while (node) {
            if (node->key == key) {
                node->value = value; // update existing
                return;
            }
            node = node->next;
        }
        // insert new node at head of chain
        ChainNode<V>* newNode = new ChainNode<V>(key, value);
        newNode->next = buckets[index];
        buckets[index] = newNode;
    }

    // Returns nullptr if key not found
    V* get(const std::string& key) {
        unsigned int index = hashFunction(key);
        ChainNode<V>* node = buckets[index];
        while (node) {
            if (node->key == key) return &node->value;
            node = node->next;
        }
        return nullptr;
    }

    bool contains(const std::string& key) {
        return get(key) != nullptr;
    }

    void remove(const std::string& key) {
        unsigned int index = hashFunction(key);
        ChainNode<V>* node = buckets[index];
        ChainNode<V>* prev = nullptr;
        while (node) {
            if (node->key == key) {
                if (prev) prev->next = node->next;
                else buckets[index] = node->next;
                delete node;
                return;
            }
            prev = node;
            node = node->next;
        }
    }

    // Apply a function to every (key, value) pair — used for health checks
    template <typename Func>
    void forEach(Func f) {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            ChainNode<V>* node = buckets[i];
            while (node) {
                f(node->key, node->value);
                node = node->next;
            }
        }
    }
};

#endif
