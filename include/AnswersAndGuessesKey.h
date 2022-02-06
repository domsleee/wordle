#pragma once
#include "WordSetUtil.h"
#include "Util.h"

struct AnswersAndGuessesKey {
    const WordSetAnswers wsAnswers;
    const WordSetGuesses wsGuesses;
    const int triesRemaining;

    AnswersAndGuessesKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, int triesRemaining)
        : wsAnswers(wsAnswers),
          wsGuesses(wsGuesses),
          triesRemaining(triesRemaining) {}

    friend bool operator==(const AnswersAndGuessesKey &a, const AnswersAndGuessesKey &b) {
        return a.wsAnswers == b.wsAnswers
          && a.wsGuesses == b.wsGuesses
          && a.triesRemaining == b.triesRemaining;
    }
};


template <>
struct std::hash<AnswersAndGuessesKey> {
    std::size_t operator()(const AnswersAndGuessesKey& k) const {
        size_t res = 17;
        res = res * 31 + hash<WordSetAnswers>()( k.wsAnswers );
        res = res * 31 + hash<WordSetGuesses>()( k.wsGuesses );
        res = res * 31 + hash<int>()( k.triesRemaining );
        return res;
    }
};

