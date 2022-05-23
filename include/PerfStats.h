#pragma once
#include <ostream>

const int NUM_FAST_MODES = 4;
const int MAX_DEPTH = 10;

struct PerfStats {
    long long entryStats[MAX_DEPTH][NUM_FAST_MODES] = {0};

    PerfStats() {
    }

    void combine(PerfStats &other) {
        for (int i = 0; i < MAX_DEPTH; ++i) {
            for (int j = 0; j < NUM_FAST_MODES; ++j) {
                entryStats[i][j] += other.entryStats[i][j];
            }
        }
    }

    friend std::ostream& operator<< (std::ostream& out, const PerfStats &curr) {
        printf("%6s %10s %10s %10s %10s\n", "rDepth", "mode0", "mode1", "mode2", "mode3");
        for (int i = 1; i <= GlobalArgs.maxTries; ++i) {
            printf("%6d %10lld %10lld %10lld %10lld\n", i, curr.entryStats[i][0], curr.entryStats[i][1], curr.entryStats[i][2], curr.entryStats[i][3]);
        }
        return out;
    }

};
