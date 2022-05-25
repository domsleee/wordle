#pragma once
#include <ostream>

const int NUM_FAST_MODES = 4;
const int MAX_DEPTH = 10;
const int TOTAL_TIMINGS = 50010;

#define TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth) (50000 + depth)

struct PerfStats {
    long long entryStats[MAX_DEPTH][NUM_FAST_MODES] = {0};
    double lcpu[TOTAL_TIMINGS] = {0.00}, tcpu[TOTAL_TIMINGS] = {0.00};
    long long ncpu[TOTAL_TIMINGS] = {0};

    PerfStats() {
    }

    void combine(PerfStats &other) {
        for (int i = 0; i < MAX_DEPTH; ++i) {
            for (int j = 0; j < NUM_FAST_MODES; ++j) {
                entryStats[i][j] += other.entryStats[i][j];
            }
        }
        for (int i = 0; i < TOTAL_TIMINGS; ++i) {
            lcpu[i] += other.lcpu[i];
            ncpu[i] += other.ncpu[i];
            tcpu[i] += other.tcpu[i];
        }
    }

    friend std::ostream& operator<< (std::ostream& out, const PerfStats &curr) {
        printf("%6s %12s %12s %12s %12s %12s\n", "depth", "mode0", "mode1", "mode2", "mode3", "#wrongFor2");
        for (int i = 1; i <= GlobalArgs.maxTries; ++i) {
            printf("%6d %12lld %12lld %12lld %12lld %12lld %2f\n", i, curr.entryStats[i][0], curr.entryStats[i][1], curr.entryStats[i][2], curr.entryStats[i][3], curr.ncpu[200+i], curr.tcpu[200+i]);
        }
        printf("numAnswers,calls,time\n");
        for (int i = 0; i < 5000; ++i) {
            auto v = 2*5000 + i;
            if (curr.ncpu[v] == 0) continue;
            DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
        }

        printf("numNonLetters,calls,time\n");
        for (int i = 0; i <= 26; ++i) {
            auto v = 2000 + i;
            if (curr.ncpu[v] == 0) continue;
            DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
        }

#define PRINT_ENTRY(label, i) DEBUG(i << " (" << label << ")" << ":" << curr.ncpu[i] << "," << curr.tcpu[i]);

        PRINT_ENTRY("yeet", 4444);
        PRINT_ENTRY("answerLoop", 4445);
        PRINT_ENTRY("getCache", 4446);
        PRINT_ENTRY("getLbCache", 4447);
        PRINT_ENTRY("getCacheKey", 4448);

        printf("RemoveGuessesBetterGuess\n");
        printf("depth,calls,time\n");
        for (int i = 1; i <= GlobalArgs.maxTries; ++i) {
            auto v = TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(i);
            DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
        }

        return out;
    }

    void tick(int i){if(GlobalArgs.timings)lcpu[i]=cpu();}
    void tock(int i){if(GlobalArgs.timings){ncpu[i]+=1;tcpu[i]+=cpu()-lcpu[i];}}
    double cpu(){return clock()/double(CLOCKS_PER_SEC);}

};
