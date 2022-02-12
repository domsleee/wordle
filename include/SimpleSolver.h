#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"

#include <algorithm>

struct SimpleSolver {
    SimpleSolver(const std::vector<std::string> &allWords): allWords(allWords) {}

    const std::vector<std::string> allWords;
    std::string firstWord = "soare";

    void precompute(){}

    int solveWord(const std::string &answer) const {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter);

        std::vector<IndexType> words(allWords.size());
        for (std::size_t i = 0; i < allWords.size(); ++i) {
            words[i] = i;
        }

        const auto answerIt = std::find(allWords.begin(), allWords.end(), answer);
        if (answerIt == allWords.end()) {
            DEBUG("error: answer not in word list");
            exit(1);
        }
        const auto answerIndex = std::distance(allWords.begin(), answerIt);

        auto guessIndex = words[0];
        const auto it = std::find(allWords.begin(), allWords.end(), firstWord);
        if (it != allWords.end()) {
            guessIndex = std::distance(allWords.begin(), it);
        }
        for (int i = 1; MAX_TRIES; ++i) {
            if (guessIndex == answerIndex) { return i; }
            if (i == MAX_TRIES) break;
            words = state.guessWord(guessIndex, words, allWords);
            guessIndex = words[0];
        }
        return -1;
    }
};
