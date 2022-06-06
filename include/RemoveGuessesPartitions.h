#pragma once
#include <vector>
#include "Util.h"
#include "PerfStats.h"
#include "PatternGetterCached.h"
#include "Defs.h"
#include "GlobalState.h"

enum CompareResult {
    BetterThanOrEqualTo,
    Unsure
};

struct RemoveGuessesPartitions {
    using PartitionVec = std::vector<std::vector<AnswersVec>>;
    static GuessesVec removeWithBetterOrSameGuess(PerfStats &stats, const GuessesVec &guesses, const AnswersVec &answers) {

        static int totalCalls = 0;
        ++totalCalls;

        PartitionVec partitions = getPartitions(guesses, answers);

        std::vector<int8_t> eliminated(GlobalState.allGuesses.size(), 0);
        auto res = guesses;
        int ct = 0;
        //std::sort(res.begin(), res.end(), [&](auto g1, auto g2) { return partitions[g1].size() > partitions[g2].size(); });

        int nGuesses = guesses.size();
        for (int i = nGuesses-1; i >= 0; --i) {
            //DEBUG("i" << i << ", ct: " << ct);
            auto g1 = res[i];
            assert(!eliminated[g1]);
            for (int j = 0; j < i; ++j) {
                auto g2 = res[j];
                if (eliminated[g2]) continue;
                auto r1 = compare(partitions, g1, g2);
                auto r2 = compare(partitions, g2, g1);

                if (r1 == BetterThanOrEqualTo && r2 == BetterThanOrEqualTo) {
                    eliminated[g1] = 1; ct++;
                } else if (r1 == BetterThanOrEqualTo) {
                    // eliminated[g2] = 1;
                } else if (r2 == BetterThanOrEqualTo) {
                    eliminated[g1] = 1; ct++;
                }

                if (eliminated[g1] || eliminated[g2]) {
                    static int betterCt = 0;
                    if (++betterCt % 100 == 0) DEBUG("betterCt: " << betterCt << ", " << totalCalls);
                }
                if (eliminated[g1]) break;
            }
        }

        std::erase_if(res, [&](const auto guessIndex) {
            return eliminated[guessIndex] == 1;
        });
        return res;
    }

    static PartitionVec getPartitions(const GuessesVec &guesses, const AnswersVec &answers) {
        PartitionVec partitions(GlobalState.allGuesses.size(), std::vector<AnswersVec>());
        for (const auto guessIndex: guesses) {
            std::vector<AnswersVec> equiv(NUM_PATTERNS, AnswersVec());
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                equiv[patternInt].push_back(answerIndex);
            }
            for (PatternType i = 0; i < NUM_PATTERNS; ++i) {
                if (equiv[i].size()) {
                    std::sort(equiv[i].begin(), equiv[i].end());
                    partitions[guessIndex].push_back(equiv[i]);
                }
            }
        }

        return partitions;
    }

    // is g1 a better guess than g2
    // if every partition p1 of P(H, g1) has a partition p2 of P(H, g2) where p1 is a subset of p2, then g1 is at least as good as g2
    static CompareResult compare(const PartitionVec &partitions, IndexType g1, IndexType g2, bool isDebug = false) {
        for (const auto &p1: partitions[g1]) {
            if (p1.size() <= 3) continue; // probably can be 3 ;)
            bool hasSubset = false;
            for (const auto &p2: partitions[g2]) {
                auto p1IsSubsetOfP2 = std::includes(p2.begin(), p2.end(), p1.begin(), p1.end());
                if (p1IsSubsetOfP2) {
                    hasSubset = true;
                    break;
                }
            }
            if (!hasSubset) {
                if (isDebug) {
                    DEBUG("g1 " << g1 << " is not a better guess than g2 " << g2 << " because:");
                    printIterable(p1);
                    DEBUG("had no subset in ");
                    printPartitions(partitions, g2);
                }
                return CompareResult::Unsure;
            }
        }
        return CompareResult::BetterThanOrEqualTo;
    }

    static void printPartitions(const PartitionVec &partitions, IndexType g1) {
        DEBUG("partitions of " << g1 << ": " << GlobalState.reverseIndexLookup[g1]);
        for (const auto &p: partitions[g1]) {
            DEBUG("----");
            std::string r = "";
            for (auto &v: p) r += std::string(",") + std::to_string(v);
            DEBUG(r);
        }
    }
};