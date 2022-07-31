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

// O(H.T^2)
struct RemoveGuessesPartitions {
    using PartitionVec = std::vector<std::vector<AnswersVec>>;
    using PartitionInvertedIndex = std::vector<int>;

    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &guesses, const AnswersVec &answers) {
        //DEBUG("H: " << answers.size() << ", T: " << guesses.size());
        const int nGuesses = guesses.size();
        stats.tick(70);
        PartitionVec partitions = getPartitionsINDEX(guesses, answers);
        stats.tock(70);
        //auto partitionInvertedIndex = buildPartitionInvertedIndex(partitions);

        stats.tick(71);
        std::vector<int8_t> eliminated(nGuesses, 0);

        for (int i = 0; i < nGuesses; ++i) {
            if (eliminated[i]) continue;
            for (int j = i+1; j < nGuesses; ++j) {
                if (eliminated[j]) continue;
                determineEliminate(eliminated, partitions, i, j);
                if (eliminated[i]) break;
            }
        }
        stats.tock(71);

        int count = 0;
        std::erase_if(guesses, [&](const auto guessIndex) {
            ++count;
            return eliminated[count-1];
        });
    }

    static void determineEliminate(std::vector<int8_t> &eliminated, const PartitionVec &partitions, int i, int j) {
        assert(i < j);

        auto r1 = compare(partitions, i, j);
        if (r1 == BetterThanOrEqualTo) {
            markAsEliminated(eliminated, j, i);
            return;
        }

        auto r2 = compare(partitions, j, i);
        if (r2 == BetterThanOrEqualTo) {
            markAsEliminated(eliminated, i, j);
        }
    }

    static PartitionInvertedIndex buildPartitionInvertedIndex(const PartitionVec &partitions) {
        const int nPartitions = partitions.size();
        const int allAnswersSize = GlobalState.allAnswers.size();
        PartitionInvertedIndex partitionInvertedIndex(nPartitions * GlobalState.allAnswers.size(), 0);
        for (int i = 0; i < nPartitions; ++i) {
            for (int pi = 0; pi < static_cast<int>(partitions[i].size()); ++pi) {
                const auto &p = partitions[i][pi];
                for (auto h1: p) {
                    partitionInvertedIndex[i * allAnswersSize + h1] = pi;
                }
            }
        }
        return partitionInvertedIndex;
    }

    static void markAsEliminated(std::vector<int8_t> &eliminated, IndexType guessToElim, IndexType guessThatIsBetter, bool equalTo = false) {
        // if (guessToElim == 751) {
        //     std::string op = equalTo ? std::string(">=") : std::string(">");
        //     DEBUG("eliminated " << guessToElim << " because " << guessThatIsBetter << ' ' << op << ' ' << guessToElim);
        // }
        eliminated[guessToElim] = 1;
    }

    static PartitionVec getPartitionsINDEX(const GuessesVec &guesses, const AnswersVec &answers) {
        const int nGuesses = guesses.size();
        PartitionVec partitions(nGuesses, std::vector<AnswersVec>());
        assert(std::is_sorted(answers.begin(), answers.end()));
        for (int i = 0; i < nGuesses; ++i) {
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guesses[i]);
                if (patternToInd[patternInt] == 255) {
                    const auto ind = partitions[i].size();
                    patternToInd[patternInt] = ind;
                    partitions[i].push_back({answerIndex});
                } else {
                    const auto ind = patternToInd[patternInt];
                    partitions[i][ind].push_back(answerIndex);
                }
            }
        }

        return partitions;
    }

    // is g1 a better guess than g2
    // if all partitions p1 of P(H, g1) has a partition p2 of P(H, g2) where p1 is a subset of p2, then g1 is at least as good as g2
    static CompareResult compare(const PartitionVec &partitions, int i, int j, bool isDebug = false) {
        // remDepth + (remDepth >= 3 ? (anyNSolvedIn2Guesses - 2) : 0)
        const int REM_DEPTH = 4; // for depth=2, remDepth=4
        const int anyNSolvedIn2Guesses = 3;
        for (const auto &p1: partitions[i]) {
            if (p1.size() <= REM_DEPTH + (REM_DEPTH >= 3 ? (anyNSolvedIn2Guesses - 2) : 0)) continue; // assumes remDepth >= 2
            bool hasSubset = false;
            for (const auto &p2: partitions[j]) {
                auto p1IsSubsetOfP2 = std::includes(p2.begin(), p2.end(), p1.begin(), p1.end());
                if (p1IsSubsetOfP2) {
                    hasSubset = true; break;
                }
            }
   
            if (!hasSubset) {
                if (isDebug) {
                    DEBUG("g1 " << i << " is not a better guess than g2 " << j << " because:");
                    printIterable(p1);
                    DEBUG("had no subset in ");
                    printPartitions(partitions, j);
                }
                return CompareResult::Unsure;
            }
        }
        return CompareResult::BetterThanOrEqualTo;
    }

    static void printPartitions(const PartitionVec &partitions, int i) {
        DEBUG("partitions of " << i);
        for (const auto &p: partitions[i]) {
            DEBUG("----");
            std::string r = "";
            for (auto &v: p) r += std::string(",") + std::to_string(v);
            DEBUG(r);
        }
    }
};