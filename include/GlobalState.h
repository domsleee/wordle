#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include "WordSetUtil.h"

struct _GlobalState {
    std::vector<std::string> allGuesses = {}, allAnswers = {};
    std::vector<std::string> reverseIndexLookup = {};

    _GlobalState(){}

    _GlobalState(
        const std::vector<std::string> &allGuesses,
        const std::vector<std::string> &allAnswers)
        : allGuesses(allGuesses),
          allAnswers(allAnswers),
          reverseIndexLookup(buildLookup())
    {
        checkWordSetSize<WordSetAnswers>("NUM_WORDS", allAnswers.size());
        checkWordSetSize<WordSetGuesses>("MAX_NUM_GUESSES", allGuesses.size());

        std::unordered_set<std::string> allGuessesSet{allGuesses.begin(), allGuesses.end()};
        for (auto ans: allAnswers) {
            if (allGuessesSet.count(ans) == 0) {
                DEBUG("possible answer is missing in guesses: " << ans);
                exit(1);
            }
        }
    }

    std::vector<std::string> buildLookup() {
        std::vector<std::string> lookup(allGuesses.size());

        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            lookup[i] = allGuesses[i];
            if (i < allAnswers.size() && allGuesses[i] != allAnswers[i]) {
                DEBUG("allGuesses[" << i << "] should equal allAnswers[" << i << "], " << allGuesses[i] << " vs " << allAnswers[i]);
                exit(1);
            }
        }

        return lookup;
    }

    IndexType getIndexForWord(const std::string &word) const {
        std::size_t firstWordIndex = std::distance(
            reverseIndexLookup.begin(),
            std::find(reverseIndexLookup.begin(), reverseIndexLookup.end(), word)
        );
        if (firstWordIndex == reverseIndexLookup.size()) {
            DEBUG("guess not in word list: " << word);
            exit(1);
        }
        return (IndexType)firstWordIndex;
    }
};

static inline _GlobalState GlobalState;
