#pragma once
#include <bitset>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <execution>
#include "AttemptStateUtil.h"
#include "UnorderedVector.h"
#include "PatternGetterCached.h"
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GuessesRemainingAfterGuessCache.h"

#define ATTEMPTSTATEFASTER_DEBUG(x) DEBUG(x)

struct AttemptStateFaster {
    AttemptStateFaster(const PatternGetterCached &getter)
      : patternGetter(getter) {}

    const PatternGetterCached patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes) const {
        auto patternInt = patternGetter.getPatternIntCached(guessIndex);
        return guessWord(guessIndex, wordIndexes, patternInt);
    }

    static std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, PatternType patternInt) {
        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) return {};

        std::vector<IndexType> res(wordIndexes.size());
        const auto &ws = GuessesRemainingAfterGuessCache::getFromCache(guessIndex, patternInt);
        int i = 0;
        for (auto wordIndex: wordIndexes) {
            if (ws[wordIndex]) res[i++] = wordIndex;
        }
        res.resize(i);

        return res;
    }

    std::size_t guessWordAndRemoveUnmatched(IndexType guessIndex, UnorderedVector<IndexType> &wordIndexes) const {
        auto patternInt = patternGetter.getPatternIntCached(guessIndex);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) {
            std::size_t removed = 0;
            for (std::size_t i = wordIndexes.size()-1; i != MAX_SIZE_VAL; --i) {
                wordIndexes.deleteIndex(i);
                removed++;
            }

            return removed;
        }

        const auto &ws = GuessesRemainingAfterGuessCache::getFromCache(guessIndex, patternInt);
        std::size_t removed = 0;
        for (std::size_t i = wordIndexes.size()-1; i != MAX_SIZE_VAL; --i) {
            auto wordIndex = wordIndexes[i];
            if (!ws[wordIndex]) {
                //DEBUG("INSPECTING " << i << ", wordIndex: " << wordIndex << ", size: " << wordIndexes.size());
                wordIndexes.deleteIndex(i);
                removed++;
            }
        }
        return removed;
    }
};