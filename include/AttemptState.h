#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "PatternGetter.h"
#include "PatternGetterCached.h"

#include "GlobalState.h"
#include "Util.h"
#include "WordSetUtil.h"
#include "PatternIntHelpers.h"
#include <unordered_map>
#include <unordered_set>
#include <cmath>

struct AttemptState {
    AttemptState(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes) const {
        auto pattern = patternGetter.getPatternFromWord(GlobalState.reverseIndexLookup[guessIndex]);
        return filterWordsMatchingGuessPattern(guessIndex, pattern, wordIndexes);
    }

    static std::vector<IndexType> filterWordsMatchingGuessPattern(IndexType guessIndex, const std::string &pattern, const std::vector<IndexType> &wordIndexes) {
        std::vector<IndexType> result = {};

        for (auto possibleAnswer: wordIndexes) {
            auto myGetter = PatternGetter(GlobalState.reverseIndexLookup[possibleAnswer]);
            auto thePattern = myGetter.getPatternFromWord(GlobalState.reverseIndexLookup[guessIndex]);
            if (thePattern == pattern) {
                result.push_back(possibleAnswer);
            }
        }

        return result;
    }

    static WordSetGuesses filterWordsMatchingGuessPattern(
        IndexType guessIndex,
        const PatternType pattern,
        const std::vector<IndexType> &wordIndexes)
    {
        WordSetGuesses result = {};
        for (auto possibleAnswer: wordIndexes) {
            auto getter = PatternGetterCached(possibleAnswer);
            const auto patternFromPossibleAnswer = getter.getPatternIntCached(guessIndex);
            if (pattern == patternFromPossibleAnswer) {
                result[possibleAnswer] = true;
            }
        }
        return result;
    }
};