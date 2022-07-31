#pragma once
#include <vector>
#include <string>

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, topLevelGuesses, maxTries, maxWrong, guessLimitPerNode, minLbCache, workers;
    static inline std::string firstWord, verify, guessesToSkip, guessesToCheck, specialLetters;
    static inline double memLimitPerThread = 2;

    // https://nerdschalk.com/what-are-most-common-letters-for-wordle/
    // eartolsincuyhpdmgbkfwvxzqj

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage;
    static inline bool disableSubsetCache = false, timings = false, pauseForAttach = false;
    static inline bool usePartitions;

    static inline int printLength;
};

static inline _GlobalArgs GlobalArgs;
