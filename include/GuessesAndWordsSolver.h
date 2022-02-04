#pragma once
#include <string>
#include <vector>
#include "PatternGetter.h"
#include "AttemptState.h"
#include <algorithm>

struct GuessesAndWordsSolver {
    GuessesAndWordsSolver(const std::vector<std::string> &answers, const std::vector<std::string> &guesses)
        : allAnswers(answers),
          allGuesses(guesses)
        {}
        
    const std::vector<std::string> allAnswers, allGuesses;

    void precompute() {}

    int solveWord(const std::string &answer) {
        auto getter = PatternGetter(answer);
        auto answersState = AttemptState(getter, allAnswers);
        auto guessesState = AttemptState(getter, allGuesses);

        auto guess = getBestWord(allAnswers, allGuesses);
        for (int i = 1; MAX_TRIES; ++i) {
            if (guess == answer) { return i; }
            if (i == MAX_TRIES) break;
            answersState = answersState.guessWord(guess);
            guessesState = guessesState.guessWord(guess);
            guess = getBestWord(answersState.words, guessesState.words);
        }
        return -1;
    }

    std::string getBestWord(const std::vector<std::string> &answers, const std::vector<std::string> &guesses) {
        auto newWords = sortByProbability(answers, guesses);
        return newWords[0];
    }

    std::vector<std::string> sortByProbability(const std::vector<std::string> &answers, const std::vector<std::string> &guesses) {
        int letterCount[26], totalLetters = 0;
        for (int i = 0; i < 26; ++i) letterCount[i] = 0;

        for (auto word: answers) {
            for (auto c: word) {
                letterCount[c-'a']++;
                totalLetters++;
            }
        }

        std::vector<WordProbPair> newWords(guesses.size());
        for (std::size_t i = 0; i < guesses.size(); ++i) {
            double prob = 0.00;
            for (auto c: guesses[i]) prob += letterCount[c-'a'];
            newWords[i] = {prob, guesses[i]};
        }
        std::sort(newWords.begin(), newWords.end(), std::greater<WordProbPair>());
        std::vector<std::string> ret(guesses.size());
        for (std::size_t i = 0; i < guesses.size(); ++i) {
            ret[i] = newWords[i].second;
        }
        return ret;
    }
};
