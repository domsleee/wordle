#pragma once
#include <vector>
#include <stack>

template<class T>
struct UnorderedVector {
    std::size_t _size = 0, maxSize = 0;
    std::vector<T> vec = {};
    std::size_t deletedStackIndex = 0;

    // for sorting
    std::vector<std::size_t> sortVec;
    std::vector<std::vector<T>> sortedStackVec;
    int sortedStackIndex = 0;

    UnorderedVector() {}

    UnorderedVector(std::size_t size)
      : _size(size),
        maxSize(size),
        vec(std::vector<T>(size*2)),
        deletedStackIndex(size),
        sortVec(size),
        sortedStackVec(10, std::vector<T>(size)) {}
    
    UnorderedVector(const std::vector<T> &inputVec)
      : UnorderedVector(inputVec.size())
    {
        for (std::size_t i = 0; i < inputVec.size(); ++i) {
            vec[i] = inputVec[i];
        }
      }
    
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
        vec[deletedStackIndex++] = i;
        _size--;
    }

    void restoreValues(std::size_t numValues) {
        assertm(_size + numValues <= maxSize,  "tried to restore more values than deleted");
        for (std::size_t i = 0; i < numValues; ++i) {
            std::swap(vec[_size], vec[vec[--deletedStackIndex]]);
            _size++;
        }
    }

    void restoreAllValues() {
        restoreValues(maxSize - _size);
    }

    void restoreToSize(std::size_t newSize) {
        assertm(newSize >= _size, "restoration size needs to be larger");
        assertm(newSize <= maxSize, "cant restore elements that don't exist");
        restoreValues(newSize - _size);
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

    void registerSortVec(T ind, std::size_t val) {
        sortVec[ind] = val;
    }

    void sortBySortVec() {
        for (std::size_t i = 0; i < _size; ++i) {
            sortedStackVec[sortedStackIndex][i] = vec[i];
        }
        std::sort(vec.begin(), vec.begin() + _size, [&](const T a, const T b) {
            return sortVec[a] < sortVec[b];
        });
        sortedStackIndex++;
    }

    void restoreSort() {
        --sortedStackIndex;
        for (std::size_t i = 0; i < _size; ++i) {
            vec[i] = sortedStackVec[sortedStackIndex][i];
        }
    }
};
