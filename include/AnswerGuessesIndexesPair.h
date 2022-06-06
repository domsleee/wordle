#pragma once
#include <vector>

template <class T>
struct AnswerGuessesIndexesPair {
    T answers;
    T guesses;
    AnswerGuessesIndexesPair(std::size_t numAnswers, std::size_t numGuesses)
        : answers(getVector<T>(numAnswers, 0)),
          guesses(getVector<T>(numGuesses, 0))
     {}
    AnswerGuessesIndexesPair(const T& answers, const T& guesses)
        : answers(answers),
          guesses(guesses)
     {}
};