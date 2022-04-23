#pragma once
#include <bitset>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <execution>
#include <unordered_map>
#include "AttemptStateFast.h"
#include "AttemptState.h"

#include "Util.h"
#define ATTEMPTSTATEFASTEST_DEBUG(x) DEBUG(x)

struct AttemptStateFastest {
    AttemptStateFastest(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = PatternIntHelpers::calcPatternInt(pattern);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) return {guessIndex};

        // vector<char>: ~50.5~ 49.83
        // vector<bool>: 68.19
        // Faster: 49.8913
        // Faster(right size): 48.73

        std::vector<IndexType> res(wordIndexes.size());
        //auto b = getBaseIndex(guessIndex, patternInt);
        int i = 0;
        for (auto wordIndex: wordIndexes) {
            if (cache[getIndex(guessIndex, patternInt, wordIndex)]) res[i++] = wordIndex;
        }
        res.resize(i);
        return res;
    }

    static inline std::size_t getBaseIndex(const std::size_t &guessIndex, int patternInt) {
        return guessIndex * NUM_PATTERNS * reverseIndexLookupSize + patternInt * reverseIndexLookupSize;
    }

    static inline std::size_t getIndex(IndexType guessIndex, int patternInt, IndexType wordIndex) {
        /*return reverseIndexLookupSizeSq * patternInt
            + reverseIndexLookupSize * wordIndex
            + guessIndex;*/
        
        /*return reverseIndexLookupSizeSq * patternInt
            + reverseIndexLookupSize * guessIndex
            + wordIndex;*/
        return guessIndex * reverseIndexLookupSize * NUM_PATTERNS + patternInt * reverseIndexLookupSize + wordIndex;
    }

    static inline std::vector<char> cache;
    static inline std::size_t reverseIndexLookupSize;
    static inline std::size_t reverseIndexLookupSizeSq;

    static void buildWSLookup(const std::vector<std::string> &reverseIndexLookup) {
        ATTEMPTSTATEFASTEST_DEBUG("buildWSLookup");
        reverseIndexLookupSize = reverseIndexLookup.size();
        reverseIndexLookupSizeSq = reverseIndexLookupSize * reverseIndexLookupSize;
        cache.assign(reverseIndexLookup.size() * reverseIndexLookup.size() * NUM_PATTERNS, {});
        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        auto wordIndexes = getVector(reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;

        int done = 0;

        for (auto guessIndex: wordIndexes) {
            DEBUG("AttemptStateFastest: " << getPerc(done, wordIndexes.size()));
            for (const auto &pattern: allPatterns) {
                auto patternInt = PatternIntHelpers::calcPatternInt(pattern);
                auto ret = AttemptStateFast::guessWord(guessIndex, wordIndexes, reverseIndexLookup, patternInt);
                for (auto ind: ret) {
                    cache[getIndex(guessIndex, patternInt, ind)] = true;
                }
            }
            done++;
        }
        
        ATTEMPTSTATEFASTEST_DEBUG("buildWSLookup finished");
    }
};