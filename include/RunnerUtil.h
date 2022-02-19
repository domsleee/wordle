#pragma once

#include "Util.h"
#include <algorithm>
#include <numeric>

template<typename T>
void printSolverInformation(const T& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << solver.cacheHit << "/" << cacheTotal << " (" << 100.00 * solver.cacheHit / cacheTotal << "%)");
}

struct RunnerUtil {
    template <typename T>
    static void printInfo(const T& solver, const std::vector<long long> &results) {
        const auto &wordsToSolve = solver.allAnswers;
        const auto &guesses = solver.allGuesses;
        auto correct = 0;
        for (auto r: results) if (r > 0) correct++;
        double avg = (double)std::reduce(results.begin(), results.end()) / correct;

        int numIncorrect = wordsToSolve.size() - correct;
        std::string valid = numIncorrect <= solver.maxIncorrect ? "Yes" : "No";

        DEBUG("=============");
        DEBUG("MAX_TRIES   : " << (int)solver.maxTries);
        DEBUG("MAX_INCORREC: " << (int)solver.maxIncorrect);
        DEBUG("valid?      : " << valid << " (" << wordsToSolve.size() << "-" << correct << " <= " << solver.maxIncorrect << ")");
        DEBUG("easy mode   : " << solver.isEasyModeVar);
        DEBUG("correct     : " << correct << "/" << wordsToSolve.size() << " (" << 100.0 * correct / wordsToSolve.size() << "%)");    
        DEBUG("guess words : " << guesses.size());
        DEBUG("average     : " << avg);
        //DEBUG("myData size : " << AttemptStateFastest::myData.size());
        printSolverInformation(solver);
    }
};

