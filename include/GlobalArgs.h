#pragma once
#include <vector>
#include <string>

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, maxTries, maxIncorrect;
    static inline std::string firstWord, guessesToSkip, guessesToCheck;

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage;
};

static inline auto GlobalArgs = _GlobalArgs();
