#pragma once
#include "WordSetUtil.h"
#include "Util.h"

struct AnswersAndGuessesKey {
    const WordSetAnswers wsAnswers;
    const WordSetGuesses wsGuesses;
    const uint8_t triesRemaining;

    AnswersAndGuessesKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, uint8_t triesRemaining)
        : wsAnswers(wsAnswers),
          wsGuesses(wsGuesses),
          triesRemaining(triesRemaining) {}

    friend bool operator==(const AnswersAndGuessesKey &a, const AnswersAndGuessesKey &b) = default;
};


template <>
struct std::hash<AnswersAndGuessesKey> {
    std::size_t operator()(const AnswersAndGuessesKey& k) const {
        size_t res = 17;
        res = res * 31 + hash<WordSetAnswers>()( k.wsAnswers );
        res = res * 31 + hash<WordSetGuesses>()( k.wsGuesses );
        res = res * 31 + hash<uint8_t>()( k.triesRemaining );
        return res;
    }
};

