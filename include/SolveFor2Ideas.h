#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"

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

    static void bigThink() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());

        auto solver = AnswersAndGuessesSolver<true, false>(2);

        DEBUG("huge think.");
        auto bar = SimpleProgress("BY_FIRST_GUESS", answers.size());

        AnswersVec myAnswers = {0,0,0};

        std::array<IndexType, NUM_PATTERNS> equivCount;
        std::array<int64_t, MAX_NUM_GUESSES> sortVec;

        for (std::size_t i = 0; i < answers.size(); ++i) {
            auto a1 = answers[i];
            myAnswers[0] = a1;
            bar.incrementAndUpdateStatus("ok");
            for (auto a2: answers) {
                if (a1 == a2) continue;
                myAnswers[1] = a2;
                for (auto a3: answers) {
                    if (a1 == a3 || a1 == a2) continue;
                    myAnswers[2] = a3;
                    auto v = solver.calcSortVectorAndGetMinNumWrongFor2(myAnswers, guesses, equivCount, sortVec, 2, 0);
                    if (v.numWrong != 0) { DEBUG("no good"); exit(1); }
                }
            }
        }

        DEBUG("magnificient"); exit(1);
    }

};