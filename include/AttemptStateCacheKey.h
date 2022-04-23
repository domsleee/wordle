#pragma once
#include "WordSetUtil.h"
#include "PatternIntHelpers.h"
#include "Util.h"

struct AttemptStateCacheKey {
    const IndexType guessIndex;
    const int pattern;

    AttemptStateCacheKey(int guessIndex, const std::string &patternStr)
        : guessIndex(guessIndex),
          pattern(PatternIntHelpers::calcPatternInt(patternStr)) {}

    friend bool operator==(const AttemptStateCacheKey &a, const AttemptStateCacheKey &b) = default;
};


template <>
struct std::hash<AttemptStateCacheKey> {
    std::size_t operator()(const AttemptStateCacheKey& k) const {
        size_t res = 17;
        res = res * 31 + hash<int>()( k.guessIndex );
        res = res * 31 + hash<int>()( k.pattern );
        return res;
    }
};

