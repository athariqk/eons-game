#pragma once

namespace ncore {

/**
 * @brief BumpAllocator defines a single contiguous block of memory on the heap.
 */
template<typename T>
class BumpAllocator {
public:
    BumpAllocator(size_t capacity) : capacity(capacity), data(new T[capacity]), size(0) {}
    ~BumpAllocator() { delete[] data; }

    T *allocate() {
        if (size >= capacity)
            return nullptr;
        return &data[size++];
    }

    T *operator[](size_t index) {
        if (index >= size)
            return nullptr;
        return &data[index];
    }

    size_t get_size() const { return size; }
    size_t get_capacity() const { return capacity; }

private:
    size_t capacity;
    T *data;
    size_t size;
};

/**
 * @brief NoobPool defines a naive-implementation of managing a growable
 * collection of reusable objects on memory heap.
 */
template<typename T>
class NoobPool {
public:
    size_t allocate() {
        if (!free_indices.empty()) {
            size_t idx = free_indices.back();
            free_indices.pop_back();
            return idx;
        }

        size_t idx = items.size();
        items.emplace_back();
        return idx;
    }

    void deallocate(size_t idx) {
        if (idx < items.size()) {
            free_indices.push_back(idx);
        }
    }

    T *get(size_t idx) {
        if (idx < items.size()) {
            return &items[idx];
        }
        return nullptr;
    }

    const T *get(size_t idx) const {
        if (idx < items.size()) {
            return &items[idx];
        }
        return nullptr;
    }

    void clear() {
        items.clear();
        free_indices.clear();
    }

    size_t capacity() const { return items.size(); }
    size_t size() const { return items.size() - free_indices.size(); }

private:
    std::vector<T> items;
    std::vector<size_t> free_indices;
};

} // namespace ncore
