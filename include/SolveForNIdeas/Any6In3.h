#pragma once
#include "Util.h"
#include "Defs.h"

struct Any6In3 {
    static void any6In3(const std::vector<AnswersVec> &list4NotIn2) {
        const auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        const auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        const auto answersToCheck = getAnswersNeededToCheck(list4NotIn2);

        //GlobalArgs.minCache = 500; // disable cache

        SimpleProgress bar("any6In3", nChoosek(answersToCheck.size()-4, 2) * list4NotIn2.size(), true);
        bar.updateStatus(FROM_SS(", verifying " << list4NotIn2.size() << " groups of 4 (#answers: " << answersToCheck.size() << ")"));
        bar.everyNth = answersToCheck.size();
        const int nAnswersToCheck = answersToCheck.size();

        auto solver = AnswersAndGuessesSolver<true>(3);
        for (const auto &l: list4NotIn2) {
            AnswersVec myAnswers = {l[0], l[1], l[2], l[3]};
            for (int i = 0; i < nAnswersToCheck; ++i) {
                const auto h1 = answersToCheck[i];
                if (std::find(myAnswers.begin(), myAnswers.end(), h1) != myAnswers.end()) continue;
                myAnswers.push_back(h1);
                for (int j = i+1; j < nAnswersToCheck; ++j) {
                    const auto h2 = answersToCheck[j];
                    bar.incrementAndUpdateStatus();
                    if (std::find(myAnswers.begin(), myAnswers.end(), h2) != myAnswers.end()) continue;
                    myAnswers.push_back(h2);
                    auto r = solver.minOverWordsLeastWrong(myAnswers, guesses, 3, 0, 1);
                    if (r.numWrong != 0) {
                        DEBUG("NO GOOD");
                        DEBUG(vecToString(myAnswers));
                    }
                    myAnswers.pop_back();
                }
                myAnswers.pop_back();
            }
        }
        bar.dispose();
        DEBUG("MAGNIFICENT");
    }

    static AnswersVec getAnswersNeededToCheck(const std::vector<AnswersVec> &list4NotIn2) {
        std::set<IndexType> mySet = {};
        for (const auto &list4: list4NotIn2) {
            for (auto h: list4) mySet.insert(h);
        }
        return {mySet.begin(), mySet.end()};
    }
};