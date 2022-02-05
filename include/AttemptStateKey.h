#pragma once
#include "WordSetUtil.h"
#include "Util.h"

template<typename T>
struct AttemptStateKey {
    const MinLetterType minLetterLimit;
    const T ws;
    const int guessIndex, answerIndex;
    AttemptStateKey(const MinLetterType &minLetterLimit, const T &ws, int guessIndex, int answerIndex)
        : minLetterLimit(minLetterLimit),
          ws(ws),
          guessIndex(guessIndex),
          answerIndex(answerIndex) {
          }

    friend bool operator==(const AttemptStateKey<T> &a, const AttemptStateKey<T> &b) {
        return a.minLetterLimit == b.minLetterLimit 
          && a.ws == b.ws
          && a.guessIndex == b.guessIndex
          && a.answerIndex == b.answerIndex;
    }
};


template <typename T>
struct std::hash<AttemptStateKey<T>> {
    std::size_t operator()(const AttemptStateKey<T>& k) const {
        size_t res = 17;
        long long ct = 0, mult = 1;
        for (int i = 0; i < 26; ++i) {
            ct += k.minLetterLimit[i] * mult;
            mult *= 5;
        }
        res = res * 31 + hash<T>()( k.ws );
        res = res * 31 + hash<int>()( k.guessIndex );
        res = res * 31 + hash<int>()( k.answerIndex );
        return res;
    }
};

