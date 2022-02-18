#pragma once
#include <vector>

template <class T>
struct AnswersGuessesIndexesPair {
    T answers;
    T guesses;
    AnswersGuessesIndexesPair(std::size_t numAnswers, std::size_t numGuesses)
        : answers(getVector<T>(numAnswers, 0)),
          guesses(getVector<T>(numAnswers, 0))
     {}
};