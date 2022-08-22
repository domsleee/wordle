#pragma once
#include "Util.h"
#include "Defs.h"

struct Any6In3 {
    static void any6In3(const std::vector<AnswersVec> &list4NotIn2) {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        SimpleProgress bar("any6In3", answers.size() * answers.size() * list4NotIn2.size(), true);
        bar.updateStatus(FROM_SS(", verifying " << list4NotIn2.size() << " groups of 4"));
        bar.everyNth = answers.size();

        auto solver = AnswersAndGuessesSolver<true>(3);
        for (const auto &l: list4NotIn2) {
            AnswersVec myAnswers = {l[0], l[1], l[2], l[3]};
            for (const auto h1: answers) {
                if (std::find(myAnswers.begin(), myAnswers.end(), h1) != myAnswers.end()) continue;
                for (const auto h2: answers) {
                    bar.incrementAndUpdateStatus();
                    if (h1 == h2) continue;
                    if (std::find(myAnswers.begin(), myAnswers.end(), h2) != myAnswers.end()) continue;
                    auto r = solver.minOverWordsLeastWrong(myAnswers, guesses, 3, 0, 1);
                    if (r.numWrong != 0) {
                        DEBUG("NO GOOD");
                        DEBUG(getIterableString(myAnswers));
                    }
                }
            }
        }
        bar.dispose();
        DEBUG("MAGNIFICENT");
    }
};