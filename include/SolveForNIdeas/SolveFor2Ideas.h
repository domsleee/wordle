#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include <bitset>
#include "SolveForNIdeas/SolveFor2Helpers.h"
#include "SolveForNIdeas/Any4In2.h"
#include "SolveForNIdeas/Any3In2.h"


struct SolveFor2Ideas {
    static void checkCanAny3BeSolvedIn2() {
        Any3In2::checkCanAny3BeSolvedIn2();
    }

    // no good batty,patty,tatty,fatty
    // no good paper,racer,waver,wafer
    static void checkCanAny4BeSolvedIn2() {
        auto solver = AnswersAndGuessesSolver<true>(2);
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        int64_t total = nChoosek(answers.size(), 4);
        Any4In2::precompute();
        std::set<std::set<IndexType>> nogood = {};

        auto bar = SimpleProgress(FROM_SS("canAny4BeSolvedIn2: " << total), GlobalState.allAnswers.size());
        int64_t ct = 0, skipped = 0, numGroups = 0;
        std::ofstream fout(FROM_SS("results/any4in2-" << GlobalState.allAnswers.size() << "-" << GlobalState.allGuesses.size() << ".txt"));
        // answers = {0};
        std::vector<int> transformResults(answers.size(), 0);
        std::mutex lock;

        std::transform(
            std::execution::par_unseq,
            answers.begin(),
            answers.end(),
            transformResults.begin(),
            [&](const IndexType a1) -> int {
                Any4In2::any4In2Fastest(solver, bar, a1, fout, lock, ct, skipped, numGroups);
                //any4Faster(solver, bar, a1, fout, lock, ct, skipped, nogood);
                return 0;
            }
        );

        DEBUG("magnificient (4)"); exit(1);
    }

private:

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

    static void any4Faster(const AnswersAndGuessesSolver<true> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, int64_t &ct, int64_t &skipped, std::set<std::set<IndexType>> &nogood) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        int64_t localCt = 0, localSkipped = 0;

        for (IndexType j = a1+1; j < nAnswers; ++j) {
            DEBUG(getPerc(j, nAnswers));
            myAnswers[1] = j;
            //DEBUG(FROM_SS("filter1: " << getPerc(filteredGuesses1.size(), guesses.size())));
            for (IndexType k = j+1; k < nAnswers; ++k) {
                myAnswers[2] = k;
                std::array<uint8_t, MAX_NUM_GUESSES> skip = {};
                int64_t maxLSkipped = nAnswers - (k+1), lSkipped = 0, lDone = 0;
                int64_t minLSkipped = maxLSkipped * 0.9;
                for (IndexType l = k+1; l < nAnswers; ++l) {
                    if (skip[l]) continue;
                    myAnswers[3] = l;
                    const auto v = solver.calcSortVectorAndGetMinNumWrongFor2RemDepth2(myAnswers, guesses, 1);
                    lDone++;
                    if (v.numWrong != 0) {
                        std::lock_guard g(lock);
                        std::set<IndexType> nogoodSet = {myAnswers.begin(), myAnswers.end()};
                        if (nogood.count(nogoodSet) == 0) {
                            nogood.insert(nogoodSet);
                            fout << setToString(nogoodSet) << '\n';
                            fout.flush();
                        }
                    } else if (lSkipped + lDone < minLSkipped) {
                        std::array<uint8_t, 243> psToAvoid = {};
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[0], v.wordIndex)] = 1;
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[1], v.wordIndex)] = 1;
                        psToAvoid[PatternGetterCached::getPatternIntCached(myAnswers[2], v.wordIndex)] = 1;

                        for (IndexType nx = l+1; nx < nAnswers; ++nx) {
                            const auto patternInt = PatternGetterCached::getPatternIntCached(nx, v.wordIndex);
                            if (!psToAvoid[patternInt]) { skip[nx] = 1; lSkipped++; }
                        }
                    }
                }
                //if (maxLSkipped > 6000) DEBUG("LSkipped: " << getPerc(lSkipped, maxLSkipped))
                localSkipped += lSkipped;
                localCt += lDone;
            }
        }
        {
            std::lock_guard g(lock);
            ct += localCt;
            skipped += localSkipped;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", skipped: " + std::to_string(skipped) + ", #groups: " + std::to_string(nogood.size()));
        }
    }
};