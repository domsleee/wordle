#pragma once

#include "Util.h"
#include <algorithm>
#include <numeric>

template<typename T>
void printSolverInformation(const T& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << getPerc(solver.cacheHit, cacheTotal));

}

struct RunnerUtil {
    template <typename T>
    static void printInfo(const T& solver, const std::vector<int64_t> &answerIndexToResult) {
        const auto &wordsToSolve = GlobalState.allAnswers;
        const auto &guesses = GlobalState.allGuesses;
        auto correct = 0;
        for (auto r: answerIndexToResult) if (r != TRIES_FAILED) correct++;
        double avg = (double)std::reduce(answerIndexToResult.begin(), answerIndexToResult.end()) / correct;

        int numIncorrect = wordsToSolve.size() - correct;
        std::string valid = getBoolVal(numIncorrect <= GlobalArgs.maxIncorrect);

        DEBUG("=============");
        DEBUG("maxTries    : " << (int)solver.maxTries);
        DEBUG("maxIncorrect: " << (int)GlobalArgs.maxIncorrect);
        DEBUG("valid?      : " << valid << " (" << numIncorrect << " <= " << GlobalArgs.maxIncorrect << ")");
        DEBUG("easy mode   : " << getBoolVal(solver.isEasyModeVar));
        DEBUG("isLowAverage: " << getBoolVal(GlobalArgs.isGetLowestAverage));
        DEBUG("correct     : " << getPerc(correct, wordsToSolve.size()));
        DEBUG("guess words : " << guesses.size());
        DEBUG("firstWord   : " << GlobalArgs.firstWord);
        DEBUG("average     : " << avg);
        //DEBUG("myData size : " << AttemptStateFastest::myData.size());
        printSolverInformation(solver);
    }

    static std::string getBoolVal(bool v) {
        return v ? "Yes" : "No";
    }
};

