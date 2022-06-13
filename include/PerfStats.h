#pragma once
#include <ostream>

const int NUM_FAST_MODES = 4;
const int MAX_DEPTH = 10;
const int TOTAL_TIMINGS = 100;

#define TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth) (60 + depth)

struct PerfStats {
    long long cacheInfo[MAX_DEPTH][3] = {0};
    long long entryStats[MAX_DEPTH][NUM_FAST_MODES] = {0};
    long long nodes = 0;
    double lcpu[TOTAL_TIMINGS] = {0.00}, tcpu[TOTAL_TIMINGS] = {0.00};
    long long ncpu[TOTAL_TIMINGS] = {0};

    PerfStats() {
    }

    void combine(PerfStats &other) {
        for (int i = 0; i < MAX_DEPTH; ++i) {
            for (int j = 0; j < NUM_FAST_MODES; ++j) {
                entryStats[i][j] += other.entryStats[i][j];
            }
            for (int j = 0; j < 3; ++j) {
                cacheInfo[i][j] += other.cacheInfo[i][j];
            }
        }
        for (int i = 0; i < TOTAL_TIMINGS; ++i) {
            lcpu[i] += other.lcpu[i];
            ncpu[i] += other.ncpu[i];
            tcpu[i] += other.tcpu[i];
        }
        nodes += other.nodes;
    }

    friend std::ostream& operator<< (std::ostream& out, const PerfStats &curr) {
        printf("%6s %12s %12s %12s %12s %12s %7s %12s %12s %12s\n", "depth", "mode0", "mode1", "mode2", "mode3", "#wrongFor2", " ", "cWrite", "cRead", "cMiss");
        for (int i = 1; i <= GlobalArgs.maxTries; ++i) {
            printf("%6d %12lld %12lld %12lld %12lld %12lld %2.4f %12lld %12lld %12lld\n", i,
                curr.entryStats[i][0], curr.entryStats[i][1], curr.entryStats[i][2], curr.entryStats[i][3],
                curr.ncpu[50+i], curr.tcpu[50+i],
                curr.cacheInfo[i][0], curr.cacheInfo[i][1], curr.cacheInfo[i][2]);
        }
        if (GlobalArgs.timings) {
            // printf("numAnswers,calls,time\n");
            // for (int i = 0; i < 5000; ++i) {
            //     auto v = 2*5000 + i;
            //     if (curr.ncpu[v] == 0) continue;
            //     DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
            // }

            // printf("numNonLetters,calls,time\n");
            // for (int i = 0; i <= 26; ++i) {
            //     auto v = 2000 + i;
            //     if (curr.ncpu[v] == 0) continue;
            //     DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
            // }

            curr.printEntry("yeet", 44);
            curr.printEntry("answerLoop", 45);
            curr.printEntry("getCache", 46);
            curr.printEntry("getCacheKey", 48);
            curr.printEntry("setCacheVal", 49);

            curr.printEntry("sumOverPart1", 20);
            curr.printEntry("sumOverPart2", 21);
            curr.printEntry("sumOverPart3", 22);
            for (int i = 0; i <= GlobalArgs.maxTries; ++i) {
                std::string s = std::string("sumOver depth:") + std::to_string(i);
                curr.printEntry(s, 23+i);
            }
            curr.printEntry("remGuesses", 33);
            curr.printEntry("remGuesses p1", 10);
            curr.printEntry("remGuesses p2", 11);
            curr.printEntry("remGuesses p3", 12);
            curr.printEntry("remGuesses p3", 13);
            curr.printEntry("lb hacks", 34);
            curr.printEntry("getGreenLetters", 35);
        }

        // printf("RemoveGuessesBetterGuess\n");
        // printf("depth,calls,time\n");
        // for (int i = 1; i <= GlobalArgs.maxTries; ++i) {
        //     auto v = TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(i);
        //     DEBUG(i << "," << curr.ncpu[v] << "," << curr.tcpu[v]);
        // }

        DEBUG("Nodes used = " << curr.nodes);
        return out;
    }

    void printEntry(std::string label, int i) const {
        printf("%5d (%15s): %12lld, %3f\n", i, label.c_str(), ncpu[i], tcpu[i]);
    }

    void addCacheWrite(RemDepthType remDepth) {
        cacheInfo[getDepth(remDepth)][0]++;

    }

    void addCacheHit(RemDepthType remDepth) {
        cacheInfo[getDepth(remDepth)][1]++;
    }

    void addCacheMiss(RemDepthType remDepth) {
        cacheInfo[getDepth(remDepth)][2]++;
    }

    int getDepth(RemDepthType remDepth) { return GlobalArgs.maxTries - remDepth; }

    void tick(int i){if(GlobalArgs.timings && !ignoreTime(i))lcpu[i]=cpu();}
    void tock(int i){if(GlobalArgs.timings && !ignoreTime(i)){ncpu[i]+=1;tcpu[i]+=cpu()-lcpu[i];}}
    bool ignoreTime(int i) {
        return (46 <= i && i <= 49);
    }
    double cpu(){return clock()/double(CLOCKS_PER_SEC);}

};
