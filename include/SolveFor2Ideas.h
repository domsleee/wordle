#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"
#include "EndGameAnalysis.h"
#include <bitset>

struct SolveFor2Ideas {
    static inline std::vector<uint8_t> numGroupsForGuessIndex;

    static void init() {
        START_TIMER(solvefor2ideas);
        numGroupsForGuessIndex.resize(GlobalState.reverseIndexLookup.size());

        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        for (const auto guessIndex: guesses) {
            int count[243] = {0};
            uint8_t numGroups = 0;
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                auto &c = count[patternInt];
                ++c;
                if (c == 1) numGroups++;
            }

            numGroupsForGuessIndex[guessIndex] = numGroups;
            //DEBUG(guessIndex << "," << (int)numGroups);
        }

        //bigThink();

        END_TIMER(solvefor2ideas);
    }

    static void checkCanAny3BeSolvedIn2() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        auto solver = AnswersAndGuessesSolver<true, false>(2);
        auto bar = SimpleProgress("canAny3BeSolvedIn2", answers.size());

        AnswersVec myAnswers = {0,0,0};
        long long ct = 0;
        
        for (auto a1: answers) {
            myAnswers[0] = a1;
            std::array<AnswersVec, 243> equiv{};
            std::array<PatternType, 243> ind{};
            int n = 0;

            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct));
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, a1);
                equiv[patternInt].push_back(answerIndex);
            }

            for (std::size_t i = 0; i < NUM_PATTERNS; ++i) {
                if (equiv[i].size()) ind[n++] = i;
            }

            for (int j = 0; j < n; ++j) {
                auto &equivJ = equiv[ind[j]];
                for (std::size_t j1 = 0; j1 < equivJ.size(); ++j1) {
                    myAnswers[1] = equivJ[j1];
                    for (std::size_t j2 = j1+1; j2 < equivJ.size(); ++j2) {
                        myAnswers[2] = equivJ[j2];
                        auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                        ++ct;
                        if (v.numWrong != 0) { bar.dispose(); DEBUG("no good " << vecToString(myAnswers)); exit(1); }
                    }
                }
            }
        }

        DEBUG("magnificient (3)"); exit(1);
    }


    #define FOR_ANY_IN_GROUP(i) for (int i_##i = 0; i_##i < n; ++i_##i) for (auto answer_##i: equiv[ind[i_##i]])
    #define FIRST_N_CONTAINS(n, val) std::find(myAnswers.begin(), myAnswers.begin()+n, val) != myAnswers.begin()+n

    // no good batty,patty,tatty,fatty
    static void checkCanAny4BeSolvedIn2() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        auto solver = AnswersAndGuessesSolver<true, false>(2);
        auto bar = SimpleProgress("canAny4BeSolvedIn2", answers.size());

        AnswersVec myAnswers = {0,0,0,0};
        long long ct = 0;

        auto wordNum2EndGameBitsets = std::vector<std::bitset<100>>(answers.size(), std::bitset<100>());
        for (auto a: answers) {
            for (auto v: EndGameAnalysis::wordNum2EndGame[a]) wordNum2EndGameBitsets[a].set(v);
        }
        
        for (auto a1: answers) {
            myAnswers[0] = a1;
            std::array<AnswersVec, 243> equiv{};
            std::array<PatternType, 243> ind{};
            int n = 0;

            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct));
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, a1);
                equiv[patternInt].push_back(answerIndex);
            }

            for (std::size_t i = 0; i < NUM_PATTERNS-1; ++i) { // the group of GGGGG has a1
                if (equiv[i].size() > 0) ind[n++] = i;
            }

            for (int j = 0; j < n; ++j) {
                const auto &equivJ = equiv[ind[j]];
                for (std::size_t j1 = 0; j1 < equivJ.size(); ++j1) {
                    myAnswers[1] = equivJ[j1];
                    for (std::size_t j2 = j1+1; j2 < equivJ.size(); ++j2) {
                        myAnswers[2] = equivJ[j2];
                        FOR_ANY_IN_GROUP(k) {
                            //DEBUG("check.." << GlobalState.reverseIndexLookup[ answer_k ] << " " << vecToString(myAnswers));
                            if (FIRST_N_CONTAINS(3, answer_k)) continue;
                            myAnswers[3] = answer_k;
                            auto bs = wordNum2EndGameBitsets[myAnswers[0]];
                            for (int i = 1; i <= 3; ++i) bs &= wordNum2EndGameBitsets[myAnswers[i]];
                            if (bs.count() > 0) continue;

                            auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                            ++ct;
                            if (v.numWrong != 0) { bar.dispose(); DEBUG("no good " << vecToString(myAnswers)); exit(1); }
                        }
                    }
                }
            }

            // for (int j = 0; j < n; ++j) {
            //     const auto &equivJ = equiv[ind[j]];
            //     for (std::size_t j1 = 0; j1 < equivJ.size(); ++j1) {
            //         myAnswers[1] = equivJ[j1];
            //         FOR_ANY_IN_GROUP(k) {
            //             if (answer_k == myAnswers[1]) continue;
            //             myAnswers[2] = answer_k;
            //             for (std::size_t j2 = j1+1; j2 < equivJ.size(); ++j2) {
            //                 if (answer_k == equivJ[j2]) continue;
            //                 myAnswers[3] = equivJ[j2];
            //                 auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
            //                 ++ct;
            //                 if (v.numWrong != 0) { bar.dispose(); DEBUG("no good " << vecToString(myAnswers)); exit(1); }
            //             }
            //         }                    
            //     }
            // }
        }

        DEBUG("magnificient (4)"); exit(1);
    }

    static std::string vecToString(const std::vector<IndexType> &indexes) {
        std::string r = GlobalState.reverseIndexLookup[indexes[0]];
        for (std::size_t i = 1; i < indexes.size(); ++i) r += "," + GlobalState.reverseIndexLookup[indexes[i]];
        return r;
    }

};