#pragma once
#include <vector>
#include <string>
#include "PatternGetter.h"
#include "Util.h"

static const char NULL_LETTER = '-';

struct AttemptState {
    AttemptState(const PatternGetter &getter, const std::vector<std::string> &words)
      : patternGetter(getter),
        tries(0),
        words(words) {
    }

    AttemptState(const PatternGetter &getter, int tries, const std::vector<std::string> &words)
      : patternGetter(getter),
        tries(tries),
        words(words) {}

    PatternGetter patternGetter;
    int tries;
    std::vector<std::string> words;
    //std::vector<std::string> deletedWords;

    AttemptState guessWord(const std::string &guess) const {
        int letterMinLimit[26] = {};
        bool excludedLetters[26] = {};
        int letterMaxLimit[26] = {};
        std::string wrongSpotPattern = std::string(guess.size(), NULL_LETTER);
        std::string rightSpotPattern = std::string(guess.size(), NULL_LETTER);

        for (int i = 0; i < 26; ++i) {
            letterMinLimit[i] = 0;
            letterMaxLimit[i] = 0;
            excludedLetters[i] = false;
        }

        auto pattern = patternGetter.getPatternFromWord(guess);

        std::size_t correct = 0;
        for (std::size_t i = 0; i < pattern.size(); ++i) {
            if (pattern[i] == '+' || pattern[i] == '?') {
                letterMinLimit[guess[i]-'a']++;
            }
            if (pattern[i] == '+') correct++;
        }
        if (correct == guess.size()) {
            return AttemptState(patternGetter, tries+1, {guess});
        }

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            rightSpotPattern[i] = (pattern[i] == '+' ? guess[i] : NULL_LETTER);
            wrongSpotPattern[i] = (pattern[i] != '+' ? guess[i] : NULL_LETTER);

            int letterInd = guess[i]-'a';
            if (pattern[i] == '_') {
                if (letterMinLimit[letterInd] == 0) {
                    excludedLetters[letterInd] = true;
                } else if (letterMaxLimit[letterInd] == 0) {
                    /*for (std::size_t j = 0; j < guess.size(); ++j) {
                        letterMaxLimit[letterInd] += (guess[j] == guess[i] && pattern[i] != '_');
                    }*/
                    letterMaxLimit[letterInd] = letterMinLimit[letterInd];
                }
            }
        }

        std::vector<std::string> result = {};
        int letterCount[26];
        for (auto word: words) {
            if (word == guess) continue;

            for (auto i = 0; i < 26; ++i) letterCount[i] = 0;
            bool allowed = true;

            for (std::size_t i = 0; i < word.size(); ++i) {
                auto letterInd = word[i]-'a';
                if (excludedLetters[letterInd]) { allowed = false; break; }
                if (wrongSpotPattern[i] == word[i]) { allowed = false; break; }
                if (rightSpotPattern[i] != NULL_LETTER && rightSpotPattern[i] != word[i]) { allowed = false; break; }
                letterCount[letterInd]++;
                if (letterMaxLimit[letterInd] != 0 && letterCount[letterInd] > letterMaxLimit[letterInd]) { allowed = false; break; }
            }

            if (!allowed) {
                for (std::size_t i = 0; i < word.size(); ++i) {
                    auto letterInd = word[i]-'a';
                    if (letterCount[letterInd] < letterMinLimit[letterInd]) { allowed = false; break; }
                }
            }

            if (allowed) {
                result.push_back(word);
            } else {
                //deletedWords.push_back(word);
            }
        }

        if (result.size() == 0) {
            DEBUG("ERROR");
            DEBUG("guess: " << guess);
            DEBUG("pattern: " << pattern);
            DEBUG("words: ");
            for (auto w: words) DEBUG(w);
            exit(1);
        }
        

        auto res = AttemptState(patternGetter, tries+1, result);
        return res;
    }
};