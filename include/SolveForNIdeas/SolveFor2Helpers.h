#pragma once
#include "util.h"
#include "Defs.h"
#include <array>

struct SolveFor2Helpers {
    static BestWordResult findAnySolutionFor2Guesses(
        const AnswersVec &answers,
        const GuessesVec &guesses)
    {
        for (const auto guessIndex: guesses) {
            if (doesGuessSeparate(answers, guessIndex)) return {0, guessIndex};
        }
        return {1, 0};
    }

    static bool doesGuessSeparate(const AnswersVec &answers, const IndexType guessIndex) {
        std::array<uint8_t, 243> seen = {};
        for (const auto answerIndex: answers) {
            const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
            if (seen[patternInt]) return false;
            seen[patternInt] = 1;
        }
        return true;
    }

    static GuessesVec filterGuessesThatSeparate(
        const AnswersVec &answers,
        const GuessesVec &guesses
    ) {
        GuessesVec res = {};
        for (const auto guessIndex: guesses) {
            if (doesGuessSeparate(answers, guessIndex)) res.push_back(guessIndex);
        }
        return res;
    }
};
