#pragma once
#include "WordSetUtil.h"
#include "WordSetHelpers.h"

#include "Util.h"

struct StateWithGuesses {
    const WordSetAnswers wsAnswers;
    const WordSetGuesses wsGuesses;
    const TriesRemainingType triesRemaining;
    StateWithGuesses(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, TriesRemainingType triesRemaining)
        : wsAnswers(wsAnswers), wsGuesses(wsGuesses), triesRemaining(triesRemaining) {}
    friend bool operator==(const StateWithGuesses &a, const StateWithGuesses &b) = default;
};

struct StateNoGuesses {
    const WordSetAnswers wsAnswers;
    const TriesRemainingType triesRemaining;
    StateNoGuesses(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, TriesRemainingType triesRemaining)
        : wsAnswers(wsAnswers), triesRemaining(triesRemaining) {}
    friend bool operator==(const StateNoGuesses &a, const StateNoGuesses &b) = default;
};

template<bool isEasyMode>
struct AnswersAndGuessesKey {
    using State = std::conditional_t<isEasyMode, StateNoGuesses, StateWithGuesses>;

    AnswersAndGuessesKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, TriesRemainingType triesRemaining)
        : state(wsAnswers, wsGuesses, triesRemaining) {}

    template<typename T>
    AnswersAndGuessesKey(const T& answers, const T& guesses, TriesRemainingType triesRemaining)
        : AnswersAndGuessesKey(
            WordSetHelpers::buildAnswersWordSet(answers),
            WordSetHelpers::buildGuessesWordSet(guesses),
            triesRemaining
        ) {}
    
    template<typename T>
    AnswersAndGuessesKey(const T& answers, TriesRemainingType triesRemaining)
        : AnswersAndGuessesKey(
            WordSetHelpers::buildAnswersWordSet(answers),
            defaultWordSetGuesses,
            triesRemaining
        ) {}
    
    constexpr static WordSetGuesses defaultWordSetGuesses = WordSetGuesses();

    // for unordered_map value
    AnswersAndGuessesKey()
        : AnswersAndGuessesKey({}, {}, 0) {}
    

    const State state;

    friend bool operator==(const AnswersAndGuessesKey<isEasyMode> &a, const AnswersAndGuessesKey<isEasyMode> &b) = default;
    friend bool operator<(const AnswersAndGuessesKey<isEasyMode> &l, const AnswersAndGuessesKey<isEasyMode> &r)
    {
        if (l.state.wsAnswers.count() != r.state.wsAnswers.count()) {
            return l.state.wsAnswers.count() < r.state.wsAnswers.count();
        }
        if constexpr (!isEasyMode) {
            if (l.state.wsGuesses.count() != r.state.wsGuesses.count()) {
                return r.state.wsGuesses.count() < r.state.wsGuesses.count();
            }
        }

        return l.state.triesRemaining < r.state.triesRemaining;
    }
};


template <bool isEasyMode>
struct std::hash<AnswersAndGuessesKey<isEasyMode>> {
    std::size_t operator()(const AnswersAndGuessesKey<isEasyMode>& k) const {
        size_t res = 17;
        res = res * 31 + hash<WordSetAnswers>()( k.state.wsAnswers );
        if constexpr (!isEasyMode) {
            res = res * 31 + hash<WordSetGuesses>()( k.state.wsGuesses );
        }
        res = res * 31 + hash<uint8_t>()( k.state.triesRemaining );
        return res;
    }
};
