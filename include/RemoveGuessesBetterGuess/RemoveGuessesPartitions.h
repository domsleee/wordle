#pragma once
#include <vector>
#include <string>
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
    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &guesses, const AnswersVec &answers) {

        static int totalCalls = 0;
        ++totalCalls;

        PartitionVec partitions = getPartitions(guesses, answers);

        std::vector<int8_t> eliminated(GlobalState.allGuesses.size(), 0);
        const bool useSort = false;
        int nGuesses = guesses.size();

        static std::vector<int> originalOrder(GlobalState.allGuesses.size(), 0);
        for (int i = 0; i < nGuesses; ++i) {
            originalOrder[guesses[i]] = i;
        }
        // if t1 is better than t2, nPartitions(t1) >= nPartitions(t2)
        if (useSort) std::sort(guesses.begin(), guesses.end(), [&](auto g1, auto g2) { return partitions[g1].size() < partitions[g2].size(); });

        for (int i = 0; i < nGuesses; ++i) {
            //DEBUG("i" << i << ", ct: " << ct);
            auto g1 = guesses[i];
            if (eliminated[g1]) continue;
            for (int j = i+1; j < nGuesses; ++j) {
                auto g2 = guesses[j];
                if (eliminated[g2]) continue;
                auto r1 = compare(partitions, g1, g2);
                auto r2 = compare(partitions, g2, g1);

                if (r1 == BetterThanOrEqualTo && r2 == BetterThanOrEqualTo) {
                    auto elimPair = originalOrder[g1] > originalOrder[g2]
                        ? std::pair<int,int>(g1, g2)
                        : std::pair<int,int>(g2, g1);
                    markAsEliminated(eliminated, elimPair.first, elimPair.second, true);
                } else if (r1 == BetterThanOrEqualTo) {
                    markAsEliminated(eliminated, g2, g1);
                } else if (r2 == BetterThanOrEqualTo) {
                    markAsEliminated(eliminated, g1, g2);
                }
                if (eliminated[g1]) break;
            }
        }

        std::erase_if(guesses, [&](const auto guessIndex) {
            return eliminated[guessIndex] == 1;
        });
        if (useSort) std::sort(guesses.begin(), guesses.end(), [&](const int i, const int j) { return originalOrder[i] < originalOrder[j]; });
    }

    static void markAsEliminated(std::vector<int8_t> &eliminated, IndexType guessToElim, IndexType guessThatIsBetter, bool equalTo = false) {
        // if (guessToElim == 751) {
        //     std::string op = equalTo ? std::string(">=") : std::string(">");
        //     DEBUG("eliminated " << guessToElim << " because " << guessThatIsBetter << ' ' << op << ' ' << guessToElim);
        // }
        eliminated[guessToElim] = 1;
    }

    static PartitionVec getPartitions(const GuessesVec &guesses, const AnswersVec &answers) {
        PartitionVec partitions(GlobalState.allGuesses.size(), std::vector<AnswersVec>());
        assert(std::is_sorted(answers.begin(), answers.end()));
        for (const auto guessIndex: guesses) {
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                if (patternToInd[patternInt] == 255) {
                    const auto ind = partitions[guessIndex].size();
                    patternToInd[patternInt] = ind;
                    partitions[guessIndex].push_back({answerIndex});
                } else {
                    const auto ind = patternToInd[patternInt];
                    partitions[guessIndex][ind].push_back(answerIndex);
                }
            }
            // std::sort(partitions[guessIndex].begin(), partitions[guessIndex].end(), [](const auto &a, const auto &b) { return a.size() < b.size(); });
        }

        return partitions;
    }

    // is g1 a better guess than g2
    // if all partitions p1 of P(H, g1) has a partition p2 of P(H, g2) where p1 is a subset of p2, then g1 is at least as good as g2
    static CompareResult compare(const PartitionVec &partitions, IndexType g1, IndexType g2, bool isDebug = false) {
        for (const auto &p1: partitions[g1]) {
            // if (p1.size() <= 3) continue; // probably can be 3 ;)
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