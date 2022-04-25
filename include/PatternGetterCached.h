#pragma once
#include <string>
#include <vector>
#include "Util.h"
#include "PatternGetter.h"

struct PatternGetterCached {
    PatternGetterCached(IndexType answerIndex): answerIndex(answerIndex) {}

    int getPatternIntCached(IndexType wordIndex) const {
        return cache[getIndex(answerIndex, wordIndex)];
    }

    static inline std::vector<PatternType> cache;
    static inline std::size_t reverseIndexLookupSize;
    static void buildCache(const std::vector<std::string> &reverseIndexLookup) {
        START_TIMER(PatternGetterCached);
        reverseIndexLookupSize = reverseIndexLookup.size();
        cache.resize(reverseIndexLookupSize * reverseIndexLookupSize);
        for (std::size_t answerIndex = 0; answerIndex < reverseIndexLookup.size(); ++answerIndex) {
            auto &answer = reverseIndexLookup[answerIndex];
            auto patternGetter = PatternGetter(answer);
            for (std::size_t wordIndex = 0; wordIndex < reverseIndexLookup.size(); ++ wordIndex) {
                auto &word = reverseIndexLookup[wordIndex];
                PatternType patternInt = patternGetter.getPatternInt(word);
                cache[getIndex(answerIndex, wordIndex)] = patternInt;
            }
        }
        END_TIMER(PatternGetterCached);
    }

    static std::size_t getIndex(IndexType answerIndex, IndexType wordIndex) {
        return answerIndex * reverseIndexLookupSize + wordIndex;
    }

  private:
    const IndexType answerIndex;
};
