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
    // no good paper,racer,waver,wafer
    static void checkCanAny4BeSolvedIn2() {
        auto solver = AnswersAndGuessesSolver<true, false>(2);
        auto bar = SimpleProgress("canAny4BeSolvedIn2", GlobalState.allAnswers.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        long long ct = 0;
        std::set<std::set<IndexType>> nogood;
        std::ofstream fout("nogood2.res");
        std::vector<int> transformResults(answers.size(), 0);
        std::mutex lock;

        std::transform(
            std::execution::par_unseq,
            answers.begin(),
            answers.end(),
            transformResults.begin(),
            [&](const IndexType a1) -> int {
                any4(solver, bar, a1, fout, lock, ct, nogood);
                return 0;
            }
        );

        DEBUG("magnificient (4)"); exit(1);
    }

    static void any4(const AnswersAndGuessesSolver<true, false> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, long long &ct, std::set<std::set<IndexType>> &nogood) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        long long localCt = 0;

        for (IndexType j = a1+1; j < nAnswers; ++j) {
            myAnswers[1] = j;
            for (IndexType k = j+1; k < nAnswers; ++k) {
                myAnswers[2] = k;
                for (IndexType l = k+1; l < nAnswers; ++l) {
                    myAnswers[3] = l;
                    auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                    localCt++;
                    if (v.numWrong != 0) {
                        std::lock_guard g(lock);
                        std::set<IndexType> nogoodSet = {myAnswers.begin(), myAnswers.end()};
                        if (nogood.count(nogoodSet) == 0) {
                            nogood.insert(nogoodSet);
                            fout << "no good " << setToString(nogoodSet) << '\n';
                            fout.flush();
                        }
                    }
                }
            }
        }

        // std::array<AnswersVec, 243> equiv{};
        // std::array<PatternType, 243> ind{};
        // int n = 0;

        // for (const auto answerIndex: answers) {
        //     const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, a1);
        //     equiv[patternInt].push_back(answerIndex);
        // }

        // for (std::size_t i = 0; i < NUM_PATTERNS-1; ++i) { // the group of GGGGG has a1
        //     if (equiv[i].size() > 0) ind[n++] = i;
        // }

        // for (int j = 0; j < n; ++j) {
        //     const auto &equivJ = equiv[ind[j]];
        //     for (std::size_t j1 = 0; j1 < equivJ.size(); ++j1) {
        //         myAnswers[1] = equivJ[j1];
        //         for (std::size_t j2 = j1+1; j2 < equivJ.size(); ++j2) {
        //             myAnswers[2] = equivJ[j2];
        //             FOR_ANY_IN_GROUP(k) {
        //                 //DEBUG("check.." << GlobalState.reverseIndexLookup[ answer_k ] << " " << vecToString(myAnswers));
        //                 if (FIRST_N_CONTAINS(3, answer_k)) continue;
        //                 myAnswers[3] = answer_k;
        //                 auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
        //                 localCt++;
        //                 if (v.numWrong != 0) {
        //                     std::lock_guard g(lock);
        //                     std::set<IndexType> nogoodSet = {myAnswers.begin(), myAnswers.end()};
        //                     if (nogood.count(nogoodSet) == 0) {
        //                         nogood.insert(nogoodSet);
        //                         fout << "no good " << vecToString(myAnswers) << '\n';
        //                         fout.flush();
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }

        {
            std::lock_guard g(lock);
            ct += localCt;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", no good: " + std::to_string(nogood.size()));
        }
    }

    static std::string setToString(const std::set<IndexType> &indexes) {
        std::vector<IndexType> conv(indexes.begin(), indexes.end());
        return vecToString(conv);
    }

    static std::string vecToString(const std::vector<IndexType> &indexes) {
        std::string r = GlobalState.reverseIndexLookup[indexes[0]];
        for (std::size_t i = 1; i < indexes.size(); ++i) r += "," + GlobalState.reverseIndexLookup[indexes[i]];
        return r;
    }
};