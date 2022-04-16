#pragma once

#include "Util.h"
#include <algorithm>
#include <numeric>

template<typename T>
void printSolverInformation(const T& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << getPerc(solver.cacheHit, cacheTotal));
    auto easierTotal = solver.easierThanProblemCacheHit + solver.easierThanProblemCacheMiss;
    DEBUG("ezThanProb c: " << getPerc(solver.easierThanProblemCacheHit, easierTotal));

}

struct RunnerUtil {
    template <typename T>
    static void printInfo(const T& solver, const std::vector<int64_t> &answerIndexToResult) {
        const auto &wordsToSolve = solver.allAnswers;
        const auto &guesses = solver.allGuesses;
        auto correct = 0;
        for (auto r: answerIndexToResult) if (r > 0) correct++;
        double avg = (double)std::reduce(answerIndexToResult.begin(), answerIndexToResult.end()) / correct;

        int numIncorrect = wordsToSolve.size() - correct;
        std::string valid = getBoolVal(numIncorrect <= solver.maxIncorrect);

        DEBUG("=============");
        DEBUG("MAX_TRIES   : " << (int)solver.maxTries);
        DEBUG("MAX_INCORREC: " << (int)solver.maxIncorrect);
        DEBUG("valid?      : " << valid << " (" << numIncorrect << " <= " << solver.maxIncorrect << ")");
        DEBUG("easy mode   : " << getBoolVal(solver.isEasyModeVar));
        DEBUG("correct     : " << correct << "/" << wordsToSolve.size() << " (" << 100.0 * correct / wordsToSolve.size() << "%)");    
        DEBUG("guess words : " << guesses.size());
        DEBUG("average     : " << avg);
        //DEBUG("myData size : " << AttemptStateFastest::myData.size());
        printSolverInformation(solver);
    }

    static std::string getBoolVal(bool v) {
        return v ? "Yes" : "No";
    }
};

