#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"

#include <algorithm>

struct SimpleSolver {
    SimpleSolver(const std::vector<std::string> &words): words(words) {}

    const std::vector<std::string> words;
    std::string firstWord = "soare";

    void precompute(){}

    int solveWord(const std::string &answer) const {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter);

        auto guess = words[0];
        if (std::find(words.begin(), words.end(), firstWord) != words.end()) {
            guess = firstWord;
        }
        for (int i = 1; MAX_TRIES; ++i) {
            if (guess == answer) { return i; }
            if (i == MAX_TRIES) break;
            auto newState = state.guessWord(guess, state.words);
            guess = newState.words[0];//getBestWord(newState.words);
            state = newState;
        }
        return -1;
    }

    std::string getBestWord(const std::vector<std::string> &words) {
        auto newWords = sortByProbability(words);
        return newWords[0];
    }

    std::vector<std::string> sortByProbability(const std::vector<std::string> &words) {
        int letterCount[26], totalLetters = 0;
        for (int i = 0; i < 26; ++i) letterCount[i] = 0;

        for (auto word: words) {
            for (auto c: word) {
                letterCount[c-'a']++;
                totalLetters++;
            }
        }

        std::vector<WordProbPair> newWords(words.size());
        for (std::size_t i = 0; i < words.size(); ++i) {
            double prob = 0.00;
            for (auto c: words[i]) prob += letterCount[c-'a'];
            newWords[i] = {prob, words[i]};
        }
        std::sort(newWords.begin(), newWords.end(), std::greater<WordProbPair>());
        std::vector<std::string> ret(words.size());
        for (std::size_t i = 0; i < words.size(); ++i) {
            ret[i] = newWords[i].second;
        }
        return ret;
    }
};
