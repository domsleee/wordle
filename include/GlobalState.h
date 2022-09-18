#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include "WordSetUtil.h"
#include "GlobalArgs.h"

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

        checkWordSetSize<WordSetAnswers>("MAX_NUM_ANSWERS", allAnswers.size());
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

struct GlobalStateContainer {
    static inline _GlobalState instance;
};

#define GlobalState GlobalStateContainer::instance

static inline void initFromGlobalArgs() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    DEBUG("INITIALISING GLOBAL STATE: T:" << guesses.size() << ", H: " << answers.size());

    GlobalState = _GlobalState(guesses, answers);
}

static inline std::string vecToString(const std::vector<IndexType> &indexes) {
    std::string r = GlobalState.reverseIndexLookup[indexes[0]];
    for (std::size_t i = 1; i < indexes.size(); ++i) r += "," + GlobalState.reverseIndexLookup[indexes[i]];
    return r;
}

static inline std::string setToString(const std::set<IndexType> &indexes) {
    std::vector<IndexType> conv(indexes.begin(), indexes.end());
    return vecToString(conv);
}

static inline std::string getAbsolutePathString(const std::string &path) {
    auto p = std::filesystem::absolute(path).string();
    replaceAll(p, "/", "_");
    return p;
}

static inline std::string getFilenameIdentifier() {
    return FROM_SS(
        GlobalState.allAnswers.size() // required to have `allAnswers` because it affects the indexes in allGuesses
        << "_" << GlobalState.allGuesses.size()
        << "__" << getAbsolutePathString(GlobalArgs.answers)
        << "__" << getAbsolutePathString(GlobalArgs.guesses));
}