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
    
    // for unordered_map value
    AnswersAndGuessesKey()
        : wsAnswers({}),
          wsGuesses({}),
          triesRemaining(0) {}

    friend bool operator==(const AnswersAndGuessesKey &a, const AnswersAndGuessesKey &b) = default;
    friend bool operator<(const AnswersAndGuessesKey& l, const AnswersAndGuessesKey& r)
    {
        if (l.wsAnswers.count() != r.wsAnswers.count()) {
            return l.wsAnswers.count() < r.wsAnswers.count();
        }
        if (l.wsGuesses.count() != r.wsGuesses.count()) {
            return r.wsGuesses.count() < r.wsGuesses.count();
        }
        return l.triesRemaining < r.triesRemaining;
    }

    // is this an easier problem
    bool isEasierThanProblem(const AnswersAndGuessesKey &harderProblem) const {
        return (wsAnswers & harderProblem.wsAnswers) == wsAnswers
         && (wsGuesses & harderProblem.wsGuesses) == wsGuesses
         && (triesRemaining >= harderProblem.triesRemaining);
    }
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

