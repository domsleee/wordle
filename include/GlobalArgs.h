#pragma once
#include <vector>
#include <string>

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, topLevelGuesses, maxTries, maxWrong, maxTotalGuesses, guessLimitPerNode;
    static inline std::string firstWord, verify, guessesToSkip, guessesToCheck;

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage;
};

static inline _GlobalArgs GlobalArgs;
