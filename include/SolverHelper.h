#pragma once
struct SolverHelper {
    static const int anyNSolvedIn2Guesses = 3; // not 4 for BIGHIDDEN: batty,patty,tatty,fatty
    static int getMaxGuaranteedSolvedInRemDepth(const int remDepth) {
        // 1 => 1
        // 2 => 3
        // 3 => 5 (not 6 for BIGHIDDEN: fills,jills,sills,vills,bills,zills)
        return 1 + ((remDepth-1) * (anyNSolvedIn2Guesses-1));
    }
};
