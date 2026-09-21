#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <utility>

// DynamicArray<T>
// DSA concept: Resizable array built from scratch (no std::vector).
// Doubles capacity when full, same idea taught for array-based lists.
// Move-aware so it can also hold non-copyable types like std::thread.
template <typename T>
class DynamicArray {
private:
    T* data;
    int capacity;
    int count;

    void resize(int newCapacity) {
        T* newData = new T[newCapacity];
        for (int i = 0; i < count; ++i) {
            newData[i] = std::move(data[i]);
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    DynamicArray() : data(nullptr), capacity(4), count(0) {
        data = new T[capacity];
    }

    ~DynamicArray() {
        delete[] data;
    }

    DynamicArray(const DynamicArray& other) : capacity(other.capacity), count(other.count) {
        data = new T[capacity];
        for (int i = 0; i < count; ++i) data[i] = other.data[i];
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this == &other) return *this;
        delete[] data;
        capacity = other.capacity;
        count = other.count;
        data = new T[capacity];
        for (int i = 0; i < count; ++i) data[i] = other.data[i];
        return *this;
    }

    void push_back(const T& value) {
        if (count == capacity) {
            resize(capacity * 2);
        }
        data[count++] = value;
    }

    // Move overload — required for non-copyable types like std::thread
    void push_back(T&& value) {
        if (count == capacity) {
            resize(capacity * 2);
        }
        data[count++] = std::move(value);
    }

    // Insert keeping array sorted by comparing with a key extractor is done
    // by the caller (HashRing); this just inserts at a specific index.
    void insertAt(int index, const T& value) {
        if (count == capacity) resize(capacity * 2);
        for (int i = count; i > index; --i) {
            data[i] = data[i - 1];
        }
        data[index] = value;
        count++;
    }

    void removeAt(int index) {
        if (index < 0 || index >= count) return;
        for (int i = index; i < count - 1; ++i) {
            data[i] = data[i + 1];
        }
        count--;
    }

    T& operator[](int index) { return data[index]; }
    const T& operator[](int index) const { return data[index]; }

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }
};

#endif
