#pragma once
#include <vector>
#include <string>

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, topLevelGuesses, maxTries, maxWrong, guessLimitPerNode;
    static inline std::string firstWord, verify, guessesToSkip, guessesToCheck, specialLetters = "eartolsincuyhpdmgbk";

    // https://nerdschalk.com/what-are-most-common-letters-for-wordle/
    // eartolsincuyhpdmgbkfwvxzqj

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage, timings = false;
};

static inline _GlobalArgs GlobalArgs;
