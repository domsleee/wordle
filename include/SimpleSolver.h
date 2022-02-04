#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"

#include <algorithm>

struct SimpleSolver {
    SimpleSolver(const std::vector<std::string> &words): words(words) {}

    const std::vector<std::string> words;

    void precompute(){}

    int solveWord(const std::string &answer) {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter, words);

        auto guess = words[0];
        if (std::find(words.begin(), words.end(), "opera") != words.end()) {
            guess = "opera";
        }
        for (int i = 1; MAX_TRIES; ++i) {
            if (guess == answer) { return i; }
            if (i == MAX_TRIES) break;
            auto newState = state.guessWord(guess);
            guess = newState.words[0];
            state = newState;
        }
        return -1;
    }
};
