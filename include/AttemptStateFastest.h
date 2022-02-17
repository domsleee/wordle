#pragma once
#include <bitset>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <execution>
#include "AttemptStateFast.h"
#include "Util.h"
#define ATTEMPTSTATEFASTEST_DEBUG(x) DEBUG(x)

const int REVERSE_INDEX_LOOKUP_SIZE = MAX_NUM_GUESSES;
using BigBitset = std::bitset<REVERSE_INDEX_LOOKUP_SIZE>;

struct AttemptStateFastest {
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
        
        ATTEMPTSTATEFASTER_DEBUG("buidWSLookup finished");
    }

};