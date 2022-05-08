#pragma once
#include "WordSetUtil.h"
#include "WordSetHelpers.h"

#include "Util.h"

struct StateWithGuesses {
    const WordSetAnswers wsAnswers;
    const WordSetGuesses wsGuesses;
    const RemDepthType remDepth;
    StateWithGuesses(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, RemDepthType remDepth)
        : wsAnswers(wsAnswers), wsGuesses(wsGuesses), remDepth(remDepth) {}
    friend bool operator==(const StateWithGuesses &a, const StateWithGuesses &b) {
        return a.remDepth == b.remDepth
            && a.wsAnswers == b.wsAnswers
            && a.wsGuesses == b.wsGuesses;
    }
};

struct StateNoGuesses {
    const WordSetAnswers wsAnswers;
    const RemDepthType remDepth;
    StateNoGuesses(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, RemDepthType remDepth)
        : wsAnswers(wsAnswers), remDepth(remDepth) {}
    friend bool operator==(const StateNoGuesses &a, const StateNoGuesses &b) {
        return a.remDepth == b.remDepth
            && a.wsAnswers == b.wsAnswers;
    }
};

template<bool isEasyMode>
struct AnswersAndGuessesKey {
    using State = std::conditional_t<isEasyMode, StateNoGuesses, StateWithGuesses>;

    AnswersAndGuessesKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, RemDepthType remDepth)
        : state(wsAnswers, wsGuesses, remDepth) {}

    template<typename T>
    AnswersAndGuessesKey(const T& answers, const T& guesses, RemDepthType remDepth)
        : AnswersAndGuessesKey(
            WordSetHelpers::buildAnswersWordSet(answers),
            WordSetHelpers::buildGuessesWordSet(guesses),
            remDepth
        ) {}
    
    template<typename T>
    AnswersAndGuessesKey(const T& answers, RemDepthType remDepth)
        : AnswersAndGuessesKey(
            WordSetHelpers::buildAnswersWordSet(answers),
            defaultWordSetGuesses,
            remDepth
        ) {}
    
    constexpr static WordSetGuesses defaultWordSetGuesses = WordSetGuesses();

    // for unordered_map value
    AnswersAndGuessesKey()
        : AnswersAndGuessesKey({}, {}, 0) {}
    

    const State state;

    friend bool operator==(const AnswersAndGuessesKey<isEasyMode> &a, const AnswersAndGuessesKey<isEasyMode> &b) = default;
};


template <bool isEasyMode>
struct std::hash<AnswersAndGuessesKey<isEasyMode>> {
    std::size_t operator()(const AnswersAndGuessesKey<isEasyMode>& k) const {
        size_t res = 17;
        res = res * 31 + hash<WordSetAnswers>()( k.state.wsAnswers );
        if constexpr (!isEasyMode) {
            res = res * 31 + hash<WordSetGuesses>()( k.state.wsGuesses );
        }
        res = res * 31 + hash<uint8_t>()( k.state.remDepth );
        return res;
    }
};
