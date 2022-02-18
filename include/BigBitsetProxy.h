#pragma once
#include <vector>

struct MyProxy {
    MyProxy(std::vector<BigBitset> &cache): cache(cache) {}
    std::vector<BigBitset> &cache;
    static const std::size_t bsSize = REVERSE_INDEX_LOOKUP_SIZE;

    auto operator[](std::size_t pos) {
        auto p = getIndex(pos);
        return cache[p.first][p.second];
    }

    bool operator[](std::size_t pos) const {
        auto p = getIndex(pos);
        return cache[p.first][p.second];
    }

    std::pair<std::size_t, std::size_t> getIndex(std::size_t pos) const {
        std::size_t ind = pos % bsSize;
        std::size_t patternInt = (pos % (bsSize * NUM_PATTERNS)) / bsSize;
        std::size_t guessIndex = pos / (NUM_PATTERNS * bsSize);

        return {guessIndex * NUM_PATTERNS + patternInt, ind};
    }
};
