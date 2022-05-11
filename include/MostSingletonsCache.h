#pragma once
#include <array>
#include "Util.h"
#include "GlobalState.h"
#include "AttemptStateFaster.h"

struct EquivGroup {
    IndexType guessIndex = 0;
    int nonZeroCt = 0;
    std::array<std::vector<IndexType>, NUM_PATTERNS> equivGroups = {};
    EquivGroup() {}
};

struct MostSingletonsCache {
    static std::array<std::size_t, NUM_WORDS> cache;
    void buildCache() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size(), 0);
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size(), 0);
        for (auto answerIndex: answers) {
            // find max #groups for `answerIndex`
            std::vector<EquivGroup> equivGroups;
            equivGroups.assign(MAX_NUM_GUESSES, {});
            for (auto guessIndex: guesses) {
                auto &equivGroup = equivGroups[guessIndex];
                equivGroup.guessIndex = guessIndex;
                getEquiv(equivGroup.equivGroups, guesses, answers, guessIndex);
                for (auto &gr: equivGroup.equivGroups) equivGroup.nonZeroCt += gr.size() > 0;
            }

            std::sort(equivGroups.begin(), equivGroups.end(), [](const auto &a, const auto &b) { return a.nonZeroCt > b.nonZeroCt; });

            for (auto &gr1: equivGroups) {
                for (auto &gr2: equivGroups) {
                    if (gr1.guessIndex == gr2.guessIndex) continue; // never makes sense.

                }
            }
        }

        // #answers = 1: return the largest #groups
        // #answers = 2: return the second largest #groups...
        // #answers = 3: return the third largest #groups... ?? fact
    }

    void getEquiv(
        std::array<std::vector<IndexType>, NUM_PATTERNS> &equiv,
        const GuessesVec &guesses,
        const AnswersVec &answers,
        const IndexType guessIndex) {

        for (std::size_t i = 0; i < NUM_PATTERNS; ++i) equiv[i].resize(0);
        for (const auto answerIndex: answers) {
            const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
            equiv[patternInt].push_back(answerIndex);
        }
    }

    // if (getMaxGroupsFrom2Guesses(numAnswers) > numAnswers) return INF;
    std::size_t getMaxGroupsFrom2Guesses(int numAnswers) {
        return cache[numAnswers];
    }
};
