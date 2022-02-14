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
    static void printInfo(const T& solver, const std::vector<long long> &results, const std::vector<std::string> &guesses, const std::vector<std::string> &wordsToSolve) {
        auto correct = 0;
        for (auto r: results) if (r > 0) correct++;
        double avg = (double)std::reduce(results.begin(), results.end()) / correct;

        DEBUG("=============");
        DEBUG("MAX_TRIES   : " << MAX_TRIES);
        DEBUG("easy mode   : " << solver.isEasyModeVar);
        DEBUG("correct     : " << correct << "/" << wordsToSolve.size() << " (" << 100.0 * correct / wordsToSolve.size() << "%)");    
        DEBUG("guess words : " << guesses.size());
        DEBUG("average     : " << avg);
        printSolverInformation(solver);
    }
};

