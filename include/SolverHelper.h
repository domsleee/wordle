#pragma once
struct SolverHelper {
    static const int anyNSolvedIn2Guesses = 3;
    static uint8_t getMaxGuaranteedSolvedInRemDepth(const int remDepth) {
        // 1 => 1
        // 2 => 3
        // 3 => 5
        return 1 + (remDepth-1) * (anyNSolvedIn2Guesses-1);
    }
};
