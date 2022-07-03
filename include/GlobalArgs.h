#pragma once
#include <vector>
#include <string>

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, topLevelGuesses, maxTries, maxWrong, guessLimitPerNode, minLbCache, workers;
    static inline std::string firstWord, verify, guessesToSkip, guessesToCheck, specialLetters = "eartolsincuyhpdmgbk";
    static inline double memLimitPerThread = 2;

    // https://nerdschalk.com/what-are-most-common-letters-for-wordle/
    // eartolsincuyhpdmgbkfwvxzqj

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage, timings = true, pauseForAttach = false;
    static inline bool useSubsetCache = true;
};

static inline _GlobalArgs GlobalArgs;
