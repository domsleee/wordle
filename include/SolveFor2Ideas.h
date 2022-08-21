#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include <bitset>
#include "SolveForNIdeas/SolveFor2Helpers.h"


// struct SolveAny4In2Guesses {
//     GuessesVec guesses;
//     AnswersVec answers;
//     const int nAnswers;
//     AnswersVec answers3 = {};
//     BestWordResult anySolution = {};
//     int numK = 0;
//     std::array<uint8_t, NUM_WORDS> seenK = {};
//     std::ofstream fout = std::ofstream("nogood3.res");
//     int nSolutions = 0;
//     std::queue<IndexType> nextCandidates = {};

//     SolveAny4In2Guesses():
//        guesses(getVector<GuessesVec>(GlobalState.allGuesses.size())),
//        answers(getVector<AnswersVec>(GlobalState.allAnswers.size())),
//        nAnswers(answers.size()) {}

//     bool any4ThatCantBeSolvedIn2() {
//         answers3.assign(3, 0);
//         SimpleProgress prog("any4ThatCantBeSolvedIn2", nAnswers);
//         for (int i = 162; i < 163; ++i) {
//             prog.incrementAndUpdateStatus(FROM_SS(" " << GlobalState.reverseIndexLookup[i]));
//             answers3[0] = answers[i];
//             for (int j = i+1; j < nAnswers; ++j) {
//                 prs(4); DEBUG("PERC2: " << getPerc(j, nAnswers));
//                 answers3[1] = answers[j];
//                 numK = 0;
//                 seenK.fill(0);

//                 for (int k = j+1; k < nAnswers; ++k) {
//                     if (seenK[k]) continue;
//                     seenK[k] = 1;
//                     checkK(k);

//                     // patternWithAnswers3[PatternGetterCached::getPatternIntCached(answers[k], anySolution.wordIndex)] = 0;
//                     while (!nextCandidates.empty()) {
//                         auto k2 = nextCandidates.front(); nextCandidates.pop();
//                         checkK(k2, true);
//                     }
//                 }
//             }
//         }

//         return false;
//     }

//     // batty(162),fatty(705),tasty,tatty,
//     bool checkK(int k, bool useSolutionFromPrev = false) {
//         answers3[2] = answers[k];
//         std::array<uint8_t, 243> patternWithAnswers3 = {};
//         // find any guess that splits all those three
//         if (!useSolutionFromPrev) anySolution = SolveFor2Helpers::findAnySolutionFor2Guesses(answers3, guesses);

//         for (const auto answerIndex: answers3) {
//             const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, anySolution.wordIndex);
//             patternWithAnswers3[patternInt] = 1;
//         }

//         const auto patternIntk = PatternGetterCached::getPatternIntCached(answers[k], anySolution.wordIndex);
//         for (int l = k+1; l < nAnswers; ++l) {
//             const auto answerIndex = answers[l];
//             const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, anySolution.wordIndex);
//             if (patternWithAnswers3[patternInt] == 0) {
//                 if (patternIntk == patternInt && !seenK[l]) { seenK[l] = 1; nextCandidates.push(l); };
//                 continue;
//             }
//             if (!seenK[l]) { seenK[l] = 1; nextCandidates.push(l); }
//             assert(!(answerIndex == answers3[0] || answerIndex == answers3[1] || answerIndex == answers3[2]));
//             answers3.push_back(answerIndex);
//             auto anySolution4 = SolveFor2Helpers::findAnySolutionFor2Guesses(answers3, guesses, 1);
//             if (anySolution4.numWrong != 0) {
//                 foundSolution(answers3);
//             }
//             answers3.pop_back();
//         }

//         return false;
//     }

//     void foundSolution(AnswersVec &answers4) {
//         DEBUG("FOUND SOLUTION #" << (++nSolutions) << ": " << getIterableString(answers3));
//         fout << vecToString(answers4) << '\n';
//         fout.flush();
//     }
// };

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

        auto solver = AnswersAndGuessesSolver<true>(2);
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
                        ++ct;
                        //auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                        //if (v.numWrong != 0) { bar.dispose(); DEBUG("no good " << vecToString(myAnswers)); exit(1); }
                        {
                            bool ok = false;
                            for (auto guessIndex: guesses) {
                                if (allPartitionsLe(guessIndex, myAnswers, 1)) { ok = true; break; }
                            }
                            if (!ok) { bar.dispose(); DEBUG("no good: " << vecToString(myAnswers)); exit(1);}
                        }
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
        auto solver = AnswersAndGuessesSolver<true>(2);
        std::set<std::set<IndexType>> nogood;
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        int64_t total = nChoosek(answers.size(), 4);
        auto bar = SimpleProgress(FROM_SS("canAny4BeSolvedIn2: " << total), GlobalState.allAnswers.size());

        int64_t ct = 0, skipped = 0;
        std::ofstream fout(FROM_SS("results/any4in2-" << GlobalState.allAnswers.size() << "-" << GlobalState.allGuesses.size() << ".txt"));
        // answers = {162};
        std::vector<int> transformResults(answers.size(), 0);
        std::mutex lock;

        std::transform(
            std::execution::par_unseq,
            answers.begin(),
            answers.end(),
            transformResults.begin(),
            [&](const IndexType a1) -> int {
                any4Faster(solver, bar, a1, fout, lock, ct, skipped, nogood);
                return 0;
            }
        );

        DEBUG("magnificient (4)"); exit(1);
    }

    static void _any4(const AnswersAndGuessesSolver<true> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, int64_t &ct, std::set<std::set<IndexType>> &nogood) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        int64_t localCt = 0;

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
                            fout << setToString(nogoodSet) << '\n';
                            fout.flush();
                        }
                    }
                }
            }
        }
        {
            std::lock_guard g(lock);
            ct += localCt;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", no good: " + std::to_string(nogood.size()));
        }
    }

    static long long nChoosek( long long n, long long k )
    {
        if (k > n) return 0;
        if (k * 2 > n) k = n-k;
        if (k == 0) return 1;

        long long result = n;
        for (long long i = 2; i <= k; ++i) {
            result *= (n-i+1);
            result /= i;
        }
        return result;
    }

    static void any4Faster(const AnswersAndGuessesSolver<true> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, int64_t &ct, int64_t &skipped, std::set<std::set<IndexType>> &nogood) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        int64_t localCt = 0, localSkipped = 0;
        std::array<uint8_t, MAX_NUM_GUESSES> skip = {};

        for (IndexType j = a1+1; j < nAnswers; ++j) {
            //DEBUG(getPerc(j, nAnswers));
            myAnswers[1] = j;
            //DEBUG(FROM_SS("filter1: " << getPerc(filteredGuesses1.size(), guesses.size())));
            for (IndexType k = j+1; k < nAnswers; ++k) {
                myAnswers[2] = k;
                std::fill(skip.begin() + k, skip.end(), 0);
                for (IndexType l = k+1; l < nAnswers; ++l) {
                    if (skip[l]) continue;
                    myAnswers[3] = l;
                    const auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                    localCt++;
                    if (v.numWrong != 0) {
                        std::lock_guard g(lock);
                        std::set<IndexType> nogoodSet = {myAnswers.begin(), myAnswers.end()};
                        if (nogood.count(nogoodSet) == 0) {
                            nogood.insert(nogoodSet);
                            fout << setToString(nogoodSet) << '\n';
                            fout.flush();
                        }
                    } else if (l == k+1) {
                        std::array<uint8_t, 243> psToAvoid = {};
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[0], v.wordIndex)] = 1;
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[1], v.wordIndex)] = 1;
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[2], v.wordIndex)] = 1;

                        for (IndexType nx = l+1; nx < nAnswers; ++nx) {
                            const auto patternInt = PatternGetterCached::getPatternIntCached(nx, v.wordIndex);
                            if (!psToAvoid[patternInt]) { skip[nx] = 1; localSkipped++; }
                        }
                    }
                }
            }
        }
        {
            std::lock_guard g(lock);
            ct += localCt;
            skipped += localSkipped;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", skipped: " + std::to_string(skipped) + ", #groups: " + std::to_string(nogood.size()));
        }
    }

    static std::string setToString(const std::set<IndexType> &indexes) {
        std::vector<IndexType> conv(indexes.begin(), indexes.end());
        return vecToString(conv);
    }

    // approximate method - try every first word
    static bool canItBeSolvedIn5() {
        checkCanAny4BeSolvedIn2(); return true;
        //return SolveAny4In2Guesses().any4ThatCantBeSolvedIn2();
    }

    // does `guessIndex` have a max partition size <= le
    static bool allPartitionsLe(const IndexType guessIndex, const AnswersVec &answers, const int le) {
        std::array<uint8_t, NUM_PATTERNS> partitionCt = {};
        for (const auto answerIndex: answers) {
            const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
            if (++partitionCt[patternInt] > le) {
                return false;
            }
        }
        return true;
    }
};