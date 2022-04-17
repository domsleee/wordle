#pragma once
#include "WordSetUtil.h"

struct WordSetHelpers
{
    template<typename Vec, typename T>
    static T buildWordSet(const Vec &wordIndexes, int offset) {
        auto wordset = T();
        for (auto wordIndex: wordIndexes) {
            wordset[offset + wordIndex] = true;
        }
        return wordset;
    }

    template<typename Vec>
    static WordSetAnswers buildAnswersWordSet(const Vec &answers) {
        return buildWordSet<Vec, WordSetAnswers>(answers, 0);
    }

    template<typename Vec>
    static WordSetGuesses buildGuessesWordSet(const Vec &guesses) {
        return buildWordSet<Vec, WordSetGuesses>(guesses, 0);
    }
};