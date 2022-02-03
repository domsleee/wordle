#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"

#include <algorithm>

struct SimpleSolver {
    int run(const std::string &answer, const std::vector<std::string> &words) {
        auto wordsOfSameLength = getWordsOfLength(words, answer.size());
        auto res = solveWord(answer, wordsOfSameLength);
        return res;
    }

    int solveWord(const std::string &answer, const std::vector<std::string> &words) {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter, words);

        auto guess = words[0];
        if (std::find(words.begin(), words.end(), "opera") != words.end()) {
            guess = "opera";
        }
        for (int i = 1; i <= 6; ++i) {
            if (guess == answer) { return i; }
            if (i == 6) break;
            auto newState = state.guessWord(guess);
            guess = newState.words[0];
            state = newState;
        }
        return -1;
    }
};
