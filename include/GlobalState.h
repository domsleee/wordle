#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include "WordSetUtil.h"

struct _GlobalState {
    std::vector<std::string> allGuesses = {}, allAnswers = {};
    std::vector<std::string> reverseIndexLookup = {};
    int maxAnswersSizeSolvedBy2 = 3; // not 2 ;)

    _GlobalState(){}

    _GlobalState(
        const std::vector<std::string> &guesses,
        const std::vector<std::string> &answers)
    {
        allAnswers = answers;
        std::sort(allAnswers.begin(), allAnswers.end());

        auto guessesCopy = guesses;
        std::sort(guessesCopy.begin(), guessesCopy.end());
        std::vector<std::string> guessesNotInAnswers = {};

        std::set_difference(guessesCopy.begin(), guessesCopy.end(), allAnswers.begin(), allAnswers.end(),
            std::inserter(guessesNotInAnswers, guessesNotInAnswers.begin()));
        
        allGuesses = allAnswers;
        for (const auto &s: guessesNotInAnswers) allGuesses.push_back(s);

        checkWordSetSize<WordSetAnswers>("NUM_WORDS", allAnswers.size());
        checkWordSetSize<WordSetGuesses>("MAX_NUM_GUESSES", allGuesses.size());

        std::unordered_set<std::string> allGuessesSet{allGuesses.begin(), allGuesses.end()};
        for (auto ans: allAnswers) {
            if (allGuessesSet.count(ans) == 0) {
                DEBUG("possible answer is missing in guesses: " << ans);
                exit(1);
            }
        }

        reverseIndexLookup = allGuesses;
    }

    void validateGuesses() {
        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            if (i < allAnswers.size() && allGuesses[i] != allAnswers[i]) {
                DEBUG("allGuesses[" << i << "] should equal allAnswers[" << i << "], " << allGuesses[i] << " vs " << allAnswers[i]);
                exit(1);
            }
        }
    }

    IndexType getIndexForWord(const std::string &word) const {
        std::size_t firstWordIndex = std::distance(
            reverseIndexLookup.begin(),
            std::find(reverseIndexLookup.begin(), reverseIndexLookup.end(), word)
        );
        if (firstWordIndex == reverseIndexLookup.size()) {
            DEBUG("word size: " << word.size());
            DEBUG("guess not in word list: " << word);
            exit(1);
        }
        return (IndexType)firstWordIndex;
    }
};

static inline _GlobalState GlobalState;
