#pragma once
#include <vector>
#include <string>
#include "Util.h"
#include "PerfStats.h"
#include "PatternGetterCached.h"
#include "RemoveGuessesPartitions.h"
#include "Defs.h"
#include "GlobalState.h"

struct RemoveGuessesPartitionsPairSeq {
    using PartitionVec = std::vector<std::vector<AnswersVec>>;
    PerfStats &stats;
    GuessesVec &guesses;
    const AnswersVec &answers;

    PartitionVec partitions;
    std::vector<std::vector<int>> partitionInvertedIndex = {};
    std::vector<std::vector<int>> partitionS = {};
    std::vector<std::vector<int>> sInverted = {};

    RemoveGuessesPartitionsPairSeq(PerfStats &_stats, GuessesVec &_guesses, const AnswersVec &_answers)
        : stats(_stats),
          guesses(_guesses),
          answers(_answers)
          {}

    static void removeWithBetterOrSameGuess(PerfStats &_stats, GuessesVec &_guesses, const AnswersVec &_answers) {
        RemoveGuessesPartitionsPairSeq(_stats, _guesses, _answers).run();
    }

private:
    void run() {
        partitions = getPartitions(); // 3 dimensional?? partitions[guess] = {{1,2},{3}}
        const int nAnswers = answers.size();
        const int nGuesses = guesses.size();

        partitionInvertedIndex.assign(partitions.size(), std::vector<int>(GlobalState.allAnswers.size()));
        for (int i = 0; i < nGuesses; ++i) {
            for (int pi = 0; pi < static_cast<int>(partitions[i].size()); ++pi) {
                const auto &p = partitions[i][pi];
                for (auto h1: p) {
                    partitionInvertedIndex[i][h1] = pi;
                }
            }
        }

        sInverted.assign(nAnswers * nAnswers, {});

        for (int i = 0; i < nGuesses; ++i) {
            for (int s1 = 0; s1 < nAnswers; s1++) {
                IndexType h1 = answers[s1];
                for (int s2 = 0; s2 < nAnswers; ++s2) {
                    IndexType h2 = answers[s2];
                    if (partitionInvertedIndex[i][h1] == partitionInvertedIndex[i][h2]) {
                        partitionS[i].push_back(s1 * nAnswers + s2);
                        sInverted[s1 * nAnswers + s2].push_back(i);
                        //if (partitionS[i].size() > sLen) break;
                    }
                }
            }
            //if (partitionS[i].size() > sLen) break;
        }

        std::vector<int8_t> eliminated(nGuesses, 0);

        for (int i = 0; i < nGuesses; ++i) {
            // DEBUG(getPerc(i, nGuesses));
            if (eliminated[i]) continue;
            std::vector<int> ctGuesses(nGuesses, 0);
            std::vector<int> nextJ = {};
            const int partitionSiSize = partitionS[i].size();
            for (int k = 0; k < partitionSiSize; ++k) {
                int sIndex = partitionS[i][k];
                for (int j: sInverted[sIndex]) {
                    if (++ctGuesses[j] == partitionSiSize) nextJ.push_back(j);
                }
            }

            for (auto j: nextJ) {
                if (eliminated[j]) continue;
                if (i == j) continue;
                determineEliminate(eliminated, i, j);
                if (eliminated[i]) break;
            }
        }

        int count = 0;
        std::erase_if(guesses, [&](const auto guessIndex) {
            ++count;
            return eliminated[count-1];
        });
    }

    void determineEliminate(std::vector<int8_t> &eliminated, int i, int j) {
        auto r1 = compare(partitions, i, j);
        auto r2 = compare(partitions, j, i);

        if (r1 == BetterThanOrEqualTo && r2 == BetterThanOrEqualTo) {
            auto elimPair = i < j
                ? std::pair<int,int>(j, i)
                : std::pair<int,int>(i, j);
            RemoveGuessesPartitions::markAsEliminated(eliminated, elimPair.first, elimPair.second, true);
        } else if (r1 == BetterThanOrEqualTo) {
            RemoveGuessesPartitions::markAsEliminated(eliminated, j, i);
        } else if (r2 == BetterThanOrEqualTo) {
            RemoveGuessesPartitions::markAsEliminated(eliminated, i, j);
        }
    }

    PartitionVec getPartitions() {
        PartitionVec partitions(guesses.size(), std::vector<AnswersVec>());
        assert(std::is_sorted(answers.begin(), answers.end()));
        for (std::size_t i = 0; i < guesses.size(); ++i) {
            auto guessIndex = guesses[i];
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
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
    CompareResult compare(const PartitionVec &partitions, int i, int j, bool isDebug = false) {
        for (const auto &p1: partitions[i]) {
            int indexJ = partitionInvertedIndex[j][p1[0]];
            const auto &p2 = partitions[j][indexJ];
            auto p1IsSubsetOfP2 = std::includes(p2.begin(), p2.end(), p1.begin(), p1.end());
            if (!p1IsSubsetOfP2) {
                return CompareResult::Unsure;
            }
        }
        return CompareResult::BetterThanOrEqualTo;
    }
};
