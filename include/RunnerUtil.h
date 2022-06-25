#pragma once

#include "Util.h"
#include "PerfStats.h"
#include <algorithm>
#include <numeric>


struct RunnerUtil {
    template <typename T>
    static void printInfo(const T& solver, const std::vector<int64_t> &answerIndexToResult) {
        printInfo(solver, answerIndexToResult, PerfStats());
    }

    template <typename T>
    static void printInfo(const T& solver, const std::vector<int64_t> &answerIndexToResult, const PerfStats &stats) {
        const auto &wordsToSolve = GlobalState.allAnswers;
        const auto &guesses = GlobalState.allGuesses;
        auto correct = 0;
        for (auto r: answerIndexToResult) if (r != TRIES_FAILED) correct++;
        const auto totalSum = std::reduce(answerIndexToResult.begin(), answerIndexToResult.end());
        double avg = (double)totalSum / correct;

        int numIncorrect = GlobalState.allAnswers.size() - correct;
        std::string valid = getBoolVal(numIncorrect <= GlobalArgs.maxWrong);

        DEBUG("=============");
        DEBUG("maxTries    : " << (int)solver.maxTries);
        DEBUG("maxWrong    : " << (int)GlobalArgs.maxWrong);
        DEBUG("valid?      : " << valid << " (" << numIncorrect << " <= " << GlobalArgs.maxWrong << ")");
        DEBUG("totalSum:   : " << totalSum);
        DEBUG("easy mode   : " << getBoolVal(solver.isEasyModeVar));
        DEBUG("isLowAverage: " << getBoolVal(GlobalArgs.isGetLowestAverage));
        DEBUG("correct     : " << getPerc(correct, wordsToSolve.size()));
        DEBUG("guess words : " << guesses.size());
        DEBUG("firstWord   : " << GlobalArgs.firstWord);
        //DEBUG("heuristcC   : " << getPerc(solver.heuristicCacheHit, solver.heuristicTotal));
        DEBUG("average     : " << avg);
        DEBUG("lettersUsed : " << GlobalArgs.specialLetters);
        DEBUG(stats);
        DEBUG("lbFromHarderInstances: " << solver.subsetCache.lbFromHarderInstances);
        //DEBUG("myData size : " << AttemptStateFastest::myData.size());
    }
};

