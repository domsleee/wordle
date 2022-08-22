#pragma once

#include "Defs.h"
#include "Util.h"
#include "GlobalState.h"

struct Any4In2 {
    static void any4In2Fastest(const AnswersAndGuessesSolver<true> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, int64_t &ct, int64_t &skipped, int64_t &numGroups) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        int64_t localCt = 0, localSkipped = 0;

        for (IndexType j = a1+1; j < nAnswers; ++j) {
            // DEBUG(getPerc(j, nAnswers));
            myAnswers[1] = j;
            for (IndexType k = j+1; k < nAnswers; ++k) {
                myAnswers[2] = k;
                if (isEliminatedBySinglesRule(a1, j, k)) {
                    localSkipped += nAnswers-(k+1);
                    continue;
                }
                //int myN = 0;
                for (IndexType l = k+1; l < nAnswers; ++l) {
                    myAnswers[3] = l;
                    std::array<PatternType, 4> patterns = {};
                    const auto wordIndex = findAnySolutionIn2(myAnswers, guesses, patterns);
                    localCt++;
                    //myN++;
                    if (wordIndex != MAX_INDEX_TYPE) {
                        for (l = l+1; l < nAnswers; ++l) {
                            const auto patternInt = PatternGetterCached::getPatternIntCached(l, wordIndex);
                            if (patternInt != patterns[0] && patternInt != patterns[1] && patternInt != patterns[2]) { localSkipped++; }
                            else { l--; break; }
                        }
                    } else {
                        std::lock_guard g(lock);
                        fout << vecToString(myAnswers) << '\n';
                        fout.flush();
                        numGroups++;
                    }
                }
                //DEBUG("myN: " << myN);
            }
        }
        {
            std::lock_guard g(lock);
            ct += localCt;
            skipped += localSkipped;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", skipped: " + std::to_string(skipped) + ", #groups: " + std::to_string(numGroups));
        }
    }

    static void any4In2Fastest2(const AnswersAndGuessesSolver<true> &solver, SimpleProgress &bar, IndexType a1, std::ofstream &fout, std::mutex &lock, int64_t &ct, int64_t &skipped, int64_t &numGroups) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        AnswersVec myAnswers = {0,0,0,0};
        const int nAnswers = answers.size();
        myAnswers[0] = a1;
        int64_t localCt = 0, localSkipped = 0;
        std::queue<IndexType> todo = {}, todoNext = {};

        for (IndexType j = a1+1; j < nAnswers; ++j) {
            DEBUG(getPerc(j, nAnswers));
            myAnswers[1] = j;
            for (IndexType k = j+1; k < nAnswers; ++k) {
                myAnswers[2] = k;
                for (IndexType l = k+1; l < nAnswers; ++l) todo.push(l);
                while (!todo.empty()) {
                    auto l = todo.front(); todo.pop();
                    myAnswers[3] = l;
                    std::array<PatternType, 4> patterns = {};
                    auto wordIndex = findAnySolutionIn2(myAnswers, guesses, patterns);
                    localCt++;
                    if (wordIndex == MAX_INDEX_TYPE) {
                        numGroups++;
                        std::lock_guard g(lock);
                        fout << vecToString(myAnswers) << '\n';
                        fout.flush();
                    } else  {
                        std::array<uint8_t, 243> psToAvoid = {};
                        psToAvoid[patterns[0]] = 1;
                        psToAvoid[patterns[1]] = 1;
                        psToAvoid[patterns[2]] = 1;

                        while (!todo.empty()) {
                            auto nx = todo.front(); todo.pop();
                            const auto patternInt = PatternGetterCached::getPatternIntCached(nx, wordIndex);
                            if (!psToAvoid[patternInt]) { localSkipped++; }
                            else todoNext.push(nx);
                        }
                        todo.swap(todoNext);
                    }
                }
            }
        }
        {
            std::lock_guard g(lock);
            ct += localCt;
            skipped += localSkipped;
            bar.incrementAndUpdateStatus(" evaluated " + std::to_string(ct) + ", skipped: " + std::to_string(skipped) + ", #groups: " + std::to_string(numGroups));
        }
    }

    static IndexType findAnySolutionIn2(const AnswersVec &answers, const GuessesVec &guesses, std::array<PatternType, 4> &patterns) {
        assert(answers.size() == 4);
        for (auto guessIndex: guesses) {
            patterns[0] = PatternGetterCached::getPatternIntCached(answers[0], guessIndex);
            patterns[1] = PatternGetterCached::getPatternIntCached(answers[1], guessIndex);
            if (patterns[1] == patterns[0]) continue;
            patterns[2] = PatternGetterCached::getPatternIntCached(answers[2], guessIndex);
            if (patterns[2] == patterns[0] || patterns[2] == patterns[1]) continue;
            patterns[3] = PatternGetterCached::getPatternIntCached(answers[3], guessIndex);
            if (patterns[3] == patterns[0] || patterns[3] == patterns[1] || patterns[3] == patterns[2]) continue;
            return guessIndex;
        }
        return MAX_INDEX_TYPE;
    }

    static bool isEliminatedBySinglesRule(IndexType answer1, IndexType answer2, IndexType answer3) {
        const auto &sA1 = singles[answer1], &sA2 = singles[answer2], &sA3 = singles[answer3];
        const int nA1 = sA1.size(), nA2 = sA2.size(), nA3 = sA3.size();
        int i = 0, j = 0, k = 0;
        for (; i < nA1; ++i) {
            while (j < nA2 && sA1[i] < sA2[j]) ++j;
            if (sA2[j] != sA1[i]) continue;
            while (k < nA3 && sA1[i] < sA3[k]) ++k;
            if (sA3[k] != sA1[i]) continue;
            return true;
        }
        return false;

        // auto locSingles = singles[i];
        // inplaceSetIntersection(locSingles, singles[j]);
        // inplaceSetIntersection(locSingles, singles[k]);

        // for ()

        // if (locSingles.size() > 0) { /*DEBUG("ELIM: " << ++ct);*/ return true; }
        // return false;
    }


    static inline GuessesVec guessesSorted = {};

    static void precompute() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        std::array<int64_t, MAX_NUM_GUESSES> sortVals = {};
        for (auto guessIndex: guesses) {
            std::array<int, NUM_PATTERNS> patternCt = {};
            std::vector<PatternType> patterns = {};
            for (auto answerIndex: answers) {
                auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                if (patternCt[patternInt]++ == 0) patterns.push_back(patternInt);
            }
            int64_t numSep = 0;
            for (std::size_t i = 0; i < patternCt.size(); ++i) {
                for (std::size_t j = i+1; j < patternCt.size(); ++j) {
                    numSep += patternCt[i] * patternCt[j];
                }
            }
            sortVals[guessIndex] = numSep;
        }

        guessesSorted = guesses;
        std::sort(guessesSorted.begin(), guessesSorted.end(), [&](const auto t1, const auto t2) { return sortVals[t1] > sortVals[t2]; });
        printf("%ld vs %ld\n", sortVals[guessesSorted[0]], sortVals[guessesSorted[1]]);

        computeSingles();
    }

    // single[i]: all test words that have {h_i} in its partition of H
    static inline std::vector<GuessesVec> singles = {};
    // singlesVec[i][j]: all test words that have {h_j} in its partition of {h_j} U {h_i..h_n}
    // static inline std::vector<std::vector<GuessesVec>> singlesVec = {};

    static void computeSingles() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        singles.assign(MAX_NUM_ANSWERS, {});
        for (auto t1: guesses) {
            std::array<int, NUM_PATTERNS> patternCt = {};
            std::vector<PatternType> patterns = {};
            for (auto hi: answers) {
                auto patternInt = PatternGetterCached::getPatternIntCached(hi, t1);
                patternCt[patternInt]++;
            }
            for (auto hi: answers) {
                auto patternInt = PatternGetterCached::getPatternIntCached(hi, t1);
                if (patternCt[patternInt] == 1) singles[hi].push_back(t1);
            }
        }

        int64_t singlesSum = 0;
        for (std::size_t answerIndex = 0; answerIndex < GlobalState.allAnswers.size(); ++answerIndex) {
            singlesSum += singles[answerIndex].size();
        }
        DEBUG("SINGLES AVG: " << (double)singlesSum / GlobalState.allAnswers.size());
    }
};