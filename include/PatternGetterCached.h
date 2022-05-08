#pragma once
#include <string>
#include <vector>
#include "Util.h"
#include "PatternGetter.h"
#include "GlobalState.h"

struct PatternGetterCached {
    PatternGetterCached(IndexType answerIndex): answerIndex(answerIndex) {}

    int getPatternIntCached(IndexType wordIndex) const {
        return cache[getIndex(answerIndex, wordIndex)];
    }

    static int getPatternIntCached(IndexType _answerIndex, IndexType wordIndex) {
        return cache[getIndex(_answerIndex, wordIndex)];
    }

    static inline std::vector<PatternType> cache;
    static inline std::size_t reverseIndexLookupSize;
    static void buildCache() {
        START_TIMER(PatternGetterCached);
        reverseIndexLookupSize = GlobalState.reverseIndexLookup.size();
        cache.resize(reverseIndexLookupSize * reverseIndexLookupSize);
        for (std::size_t answerIndex = 0; answerIndex < reverseIndexLookupSize; ++answerIndex) {
            auto &answer = GlobalState.reverseIndexLookup[answerIndex];
            auto patternGetter = PatternGetter(answer);
            for (std::size_t guessIndex = 0; guessIndex < reverseIndexLookupSize; ++guessIndex) {
                auto &word = GlobalState.reverseIndexLookup[guessIndex];
                PatternType patternInt = patternGetter.getPatternInt(word);
                cache[getIndex(answerIndex, guessIndex)] = patternInt;
            }
        }
        END_TIMER(PatternGetterCached);
    }

    static std::size_t getIndex(IndexType answerIndex, IndexType guessIndex) {
        return answerIndex * reverseIndexLookupSize + guessIndex;
    }

  private:
    const IndexType answerIndex;
};
