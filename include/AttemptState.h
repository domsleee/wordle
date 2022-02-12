#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "PatternGetter.h"
#include "Util.h"
#include "WordSetUtil.h"
#include "AttemptStateCacheKey.h"
#include <unordered_map>
#include <unordered_set>
#include <cmath>

struct AttemptState {
    using CacheType = std::unordered_map<AttemptStateCacheKey, std::unordered_set<int>>;

    AttemptState(const PatternGetter &getter)
      : patternGetter(getter) {}
    
    AttemptState(const PatternGetter &getter, const std::vector<std::string> &words)
      : patternGetter(getter),
        words(words) {}

    PatternGetter patternGetter;
    std::vector<std::string> words;

    AttemptState guessWord(const std::string &guess, const std::vector<std::string> &words) const {
        auto pattern = patternGetter.getPatternFromWord(guess);
        return guessWord(guess, pattern, words);
    }

    AttemptState guessWord(const std::string &guess, const std::string &pattern, const std::vector<std::string> &words) const {
        static MinLetterType letterMinLimit = {}, letterMaxLimit = {};
        static bool excludedLetters[26] = {};
        static auto wrongSpotPattern = std::string(guess.size(), NULL_LETTER);
        static auto rightSpotPattern = std::string(guess.size(), NULL_LETTER);

        for (int i = 0; i < 26; ++i) {
            letterMinLimit[i] = letterMaxLimit[i] = 0;
            excludedLetters[i] = false;
        }

        std::size_t correct = 0;
        for (std::size_t i = 0; i < pattern.size(); ++i) {
            if (pattern[i] == '+' || pattern[i] == '?') {
                auto ind = guess[i]-'a';
                letterMinLimit[ind]++;
            }
            if (pattern[i] == '+') correct++;
        }
        if (correct == guess.size()) {
            return AttemptState(patternGetter, {guess});
        }

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            rightSpotPattern[i] = (pattern[i] == '+' ? guess[i] : NULL_LETTER);
            wrongSpotPattern[i] = (pattern[i] == '?' ? guess[i] : NULL_LETTER);

            int letterInd = guess[i]-'a';
            if (pattern[i] == '_') {
                if (letterMinLimit[letterInd] == 0) {
                    excludedLetters[letterInd] = true;
                } else if (letterMaxLimit[letterInd] == 0) {
                    letterMaxLimit[letterInd] = letterMinLimit[letterInd];
                }
            }
        }

        std::vector<std::string> result = {};
        for (const auto &word: words) {
            MinLetterType letterCount = {};
            //for (auto i = 0; i < 26; ++i) letterCount[i] = 0;
            bool allowed = true;

            for (std::size_t i = 0; i < word.size(); ++i) {
                auto letterInd = word[i]-'a';
                if (excludedLetters[letterInd]) { allowed = false; break; }
                if (wrongSpotPattern[i] == word[i]) { allowed = false; break; }
                if (rightSpotPattern[i] != NULL_LETTER && rightSpotPattern[i] != word[i]) { allowed = false; break; }
                letterCount[letterInd]++;
                if (letterMaxLimit[letterInd] != 0 && letterCount[letterInd] > letterMaxLimit[letterInd]) { allowed = false; break; }
            }

            if (allowed) {
                for (char c: guess) {
                    auto letterInd = c-'a';
                    if (letterCount[letterInd] < letterMinLimit[letterInd]) { allowed = false; break; }
                }
            }

            if (allowed && word != guess) {
                result.push_back(word);
            }
        }

        if (result.size() == 0 && !suppressErrors) {
            DEBUG("ERROR no results when guessing word!");
            DEBUG("guess: " << guess);
            DEBUG("pattern: " << pattern);
            DEBUG("words: ");
            //for (auto w: words) DEBUG(w);
            exit(1);
        }

        auto res = AttemptState(patternGetter);
        res.words = std::move(result);
        return res;
    }

    AttemptState guessWordCached(const std::string &guess, const std::vector<std::string> &words) {
        return guessWord(guess, words);
        auto ret = AttemptState(patternGetter);
        auto pattern = patternGetter.getPatternFromWord(guess);
        auto key = AttemptStateCacheKey(reverseGuessToIndex[guess], pattern);
        const auto &allowedWords = attemptStateCache[key];
        for (const auto &wordToCheck: words) {
            auto ind = reverseGuessToIndex[wordToCheck];
            if (allowedWords.count(ind) == 1) {
                ret.words.push_back(wordToCheck);
            }
        }
        return ret;
    }

    using ReverseIndexType = std::unordered_map<std::string, int>;
    static inline ReverseIndexType reverseGuessToIndex;

    static void precompute(const std::vector<std::string> &guesses) {
        DEBUG("precomputing AttemptState");
        reverseGuessToIndex = {};
        return;
        suppressErrors = true;
        auto patterns = getAllPatterns(guesses[0].size());

        for (std::size_t i = 0; i < guesses.size(); ++i) {
            const auto &guess = guesses[i];
            reverseGuessToIndex[guess] = i;
        }

        auto dummyAttemptState = AttemptState(PatternGetter(""));
        for (std::size_t i = 0; i < guesses.size(); ++i) {
            //DEBUG("guessing " << getPerc(i, guesses.size()));
            const auto &guess = guesses[i];
            for (const auto &pattern: patterns) {
                std::unordered_set<int> allowedWords = {};
                auto newState = dummyAttemptState.guessWord(guess, pattern, guesses);
                for (const auto &w: newState.words) {
                    allowedWords.insert(reverseGuessToIndex[w]);
                }
                //allowedWords = {newState.words.begin(), newState.words.end()};
                auto key = AttemptStateCacheKey(i, pattern);
                attemptStateCache[key] = allowedWords;
            }
        }
        suppressErrors = false;

        //exit(1);
    }

    static std::vector<std::string> getAllPatterns(int length) {
        auto max = std::pow(3, length);
        std::vector<std::string> res(max, std::string(length, NULL_LETTER));
        for (auto i = 0; i < max; ++i) {
            auto v = i;
            auto &pattern = res[i];
            for (auto j = 0; j < length; ++j) {
                pattern[j] = intToChar(v%3);
                v /= 3;
            }
        }
        return res;
    }

    static char intToChar(int v) {
        switch(v) {
            case 0: return '?';
            case 1: return '_';
            case 2: return '+';
            default: {
                DEBUG("UNKNOWN INT " << v);
                exit(1);
            }
        }
    }

    static inline CacheType attemptStateCache = {};
    static inline long long cacheSize, cacheHit, cacheMiss;
    static inline bool suppressErrors = false;
    static void setupWordCache(int numIndexes) {
        // do nothing?
    }
};