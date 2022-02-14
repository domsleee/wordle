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

    PatternGetter patternGetter;
    static const bool suppressErrors = false;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        return guessWord(guessIndex, pattern, wordIndexes, wordIndexLookup);
    }

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::string &pattern, const std::vector<IndexType> wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        static MinLetterType letterMinLimit = {}, letterMaxLimit = {};
        static auto wrongSpotPattern = std::string(wordIndexLookup[0].size(), NULL_LETTER);
        static auto rightSpotPattern = std::string(wordIndexLookup[0].size(), NULL_LETTER);

        const auto &guess = wordIndexLookup[guessIndex];
        for (int i = 0; i < 26; ++i) {
            letterMinLimit[i] = 0;
            letterMaxLimit[i] = 10;
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
            return {guessIndex};
        }

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            rightSpotPattern[i] = (pattern[i] == '+' ? guess[i] : NULL_LETTER);
            wrongSpotPattern[i] = (pattern[i] == '?' ? guess[i] : NULL_LETTER);

            int letterInd = guess[i]-'a';
            if (pattern[i] == '_') {
                if (letterMinLimit[letterInd] == 0) {
                    letterMaxLimit[letterInd] = 0;
                } else if (letterMaxLimit[letterInd] == 10) {
                    letterMaxLimit[letterInd] = letterMinLimit[letterInd];
                }
            }
        }

        std::vector<IndexType> result = {};
        for (const auto &wordIndex: wordIndexes) {
            const auto &word = wordIndexLookup[wordIndex];
            MinLetterType letterCount = {};
            //for (auto i = 0; i < 26; ++i) letterCount[i] = 0;
            bool allowed = true;

            for (std::size_t i = 0; i < word.size(); ++i) {
                auto letterInd = word[i]-'a';
                if (wrongSpotPattern[i] == word[i]) { allowed = false; break; }
                if (rightSpotPattern[i] != NULL_LETTER && rightSpotPattern[i] != word[i]) { allowed = false; break; }
                letterCount[letterInd]++;
                if (letterCount[letterInd] > letterMaxLimit[letterInd]) { allowed = false; break; }
            }

            if (allowed) {
                for (char c: guess) {
                    auto letterInd = c-'a';
                    if (letterCount[letterInd] < letterMinLimit[letterInd]) { allowed = false; break; }
                }
            }

            if (allowed && word != guess) {
                result.push_back(wordIndex);
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

        return result;
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
};