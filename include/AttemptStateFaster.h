#pragma once
#include <bitset>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <execution>
#include "AttemptStateFast.h"
#include "AttemptStateUtil.h"

#include "Util.h"
#define ATTEMPTSTATEFASTER_DEBUG(x) DEBUG(x)

const int REVERSE_INDEX_LOOKUP_SIZE = MAX_NUM_GUESSES;
using BigBitset = std::bitset<REVERSE_INDEX_LOOKUP_SIZE>;

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

    std::pair<std::size_t, int> getIndex(std::size_t pos) const {
        std::size_t ind = pos % bsSize;
        std::size_t patternInt = (pos % (bsSize * NUM_PATTERNS)) / bsSize;
        std::size_t guessIndex = pos / (NUM_PATTERNS * bsSize);

        return {guessIndex * NUM_PATTERNS + patternInt, ind};
    }
};

struct AttemptStateFaster {
    AttemptStateFaster(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) return {guessIndex};

        std::vector<IndexType> res = {};
        res.reserve(wordIndexes.size());
        const auto &ws = cache[NUM_PATTERNS * guessIndex + patternInt];
        for (auto wordIndex: wordIndexes) {
            if (ws[wordIndex]) res.emplace_back(wordIndex);
        }

        return res;
    }

    static inline std::vector<BigBitset> cache;
    static void buidWSLookup(const std::vector<std::string> &reverseIndexLookup) {
        ATTEMPTSTATEFASTER_DEBUG("buildWSLookup");
        cache.resize(REVERSE_INDEX_LOOKUP_SIZE * NUM_PATTERNS);
        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        auto wordIndexes = getVector(reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;
        std::atomic<int> done = 0;

        std::string filename = "databases/test.out";

        std::transform(
            std::execution::par,
            wordIndexes.begin(),
            wordIndexes.end(),
            dummy.begin(),
            [
                &cache,
                &done,
                &allPatterns = std::as_const(allPatterns),
                &wordIndexes = std::as_const(wordIndexes),
                &reverseIndexLookup = std::as_const(reverseIndexLookup)
            ](const int guessIndex) -> int {
                done++;
                DEBUG("PROGRESS: " << getPerc(done.load(), reverseIndexLookup.size()));
                for (const auto &pattern: allPatterns) {
                    auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);
                    BigBitset ws = {};
                    auto ret = AttemptStateFast::guessWord(guessIndex, wordIndexes, reverseIndexLookup, patternInt);
                    for (auto ind: ret) ws[ind] = true;
                    cache[guessIndex * NUM_PATTERNS + patternInt] = ws;
                }
                return 0;
            }
        );
        
        AttemptStateUtil::writeToFile(MyProxy(cache), (int64_t)REVERSE_INDEX_LOOKUP_SIZE * REVERSE_INDEX_LOOKUP_SIZE * NUM_PATTERNS  , filename);
        ATTEMPTSTATEFASTER_DEBUG("buidWSLookup finished");
    }

};