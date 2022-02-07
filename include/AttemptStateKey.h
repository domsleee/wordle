#pragma once
#include "WordSetUtil.h"
#include "Util.h"

template<typename T>
struct AttemptStateKey {
    const T ws;
    const int guessIndex, answerIndex;
    AttemptStateKey(const T &ws, int guessIndex, int answerIndex)
        : ws(ws),
          guessIndex(guessIndex),
          answerIndex(answerIndex) {
          }

    friend bool operator==(const AttemptStateKey<T> &a, const AttemptStateKey<T> &b) = default;
};


template <typename T>
struct std::hash<AttemptStateKey<T>> {
    std::size_t operator()(const AttemptStateKey<T>& k) const {
        size_t res = 17;
        res = res * 31 + hash<T>()( k.ws );
        res = res * 31 + hash<int>()( k.guessIndex );
        res = res * 31 + hash<int>()( k.answerIndex );
        return res;
    }
};

