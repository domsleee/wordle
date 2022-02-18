#pragma once
#include <vector>
#include <stack>

template<class T>
struct UnorderedVector {
    std::size_t _size, maxSize;
    std::vector<T> vec;
    std::stack<T> deletedStack;

    UnorderedVector(std::size_t size)
      : _size(size),
        maxSize(size),
        vec(std::vector<T>(size)) {}
    
    std::size_t size() const { return _size; }

    auto operator[](std::size_t pos) const {
        return vec[pos];
    }

    auto& operator[](std::size_t pos) {
        return vec[pos];
    }

    void deleteIndex(std::size_t i) {
        assertm(_size >= 1, "needs to have at least 1 element to delete");
        assertm(i < _size, "index must be less than _size");
        std::swap(vec[i], vec[_size-1]);
        deletedStack.push(i);
        _size--;
    }

    void restoreValues(std::size_t numValues) {
        assertm(numValues <= deletedStack.size(),  "tried to restore more values than deleted");
        for (std::size_t i = 0; i < numValues; ++i) {
            std::swap(vec[_size], vec[deletedStack.top()]);
            _size++;
            deletedStack.pop();
        }
    }

    auto begin() const {
        return vec.begin();
    }

    auto end() const {
        return vec.begin() + _size;
    }

    auto cbegin() const {
        return vec.cbegin();
    }

    auto cend() const {
        return vec.cbegin() + _size;
    }
};
