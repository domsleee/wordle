#pragma once
#include "WordSetUtil.h"
#include "Util.h"

struct AttemptStateCacheKey {
    const int guessIndex;
    const int pattern;

    AttemptStateCacheKey(int guessIndex, const std::string &patternStr)
        : guessIndex(guessIndex),
          pattern(calcPattern(patternStr)) {}
    
    int calcPattern(const std::string &patternStr) {
        int res = 0, mult = 1;
        for (char c: patternStr) {
            int v = charToInt(c);
            res += mult * v;
            mult *= 3;
        }
        return res;
    }

    static int charToInt(char c) {
        switch(c) {
            case '?': return 0;
            case '_': return 1;
            case '+': return 2;
            default: {
                DEBUG("UNKONWN CHAR IN PATTERN STR " << c);
                exit(1);
            }
        }
    }

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

