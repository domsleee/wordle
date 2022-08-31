#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include <bitset>
#include "SolveForNIdeas/SolveFor2Helpers.h"


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