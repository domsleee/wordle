#pragma once
#include <vector>
#include <string>
#include "Defs.h"

struct _GlobalArgs {
    static inline std::string guesses, answers;

    // values
    static inline int numToRestrict, topLevelGuesses, topLevelSkip, maxTries, maxWrong, guessLimitPerNode, minLbCache, minCache, workers;
    static inline std::string firstWord, verify, guessesToSkip, guessesToCheck, specialLetters, outputRes;
    static inline double memLimitPerThread = 2;
    static inline int mPartitionsRemDepth, MPartitionsRemDepth;

    // https://nerdschalk.com/what-are-most-common-letters-for-wordle/
    // eartolsincuyhpdmgbkfwvxzqj

    // flags
    static inline bool parallel, reduceGuesses, forceSequential, hardMode, isGetLowestAverage, timings;
    static inline bool disableSubsetCache = false, disableEndGameAnalysis = false, pauseForAttach = false;
    static inline bool usePartitions;

    static inline int printLength;
    static inline OtherProgram otherProgram = OtherProgram::blank;
};

struct GlobalArgsContainer {
    static inline _GlobalArgs instance;
};

#define GlobalArgs GlobalArgsContainer::instance
