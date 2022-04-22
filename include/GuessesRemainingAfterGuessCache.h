#pragma once
#include <vector>
#include <algorithm>
#include <execution>
#include "Defs.h"
#include "WordSetUtil.h"
#include "SimpleProgress.h"

// the possible guesses that remain after guessing a word and receiving a pattern
struct GuessesRemainingAfterGuessCache {
    static inline std::vector<WordSetGuesses> cache;
    static void buildCache(const std::vector<std::string> &reverseIndexLookup) {
        auto bar = SimpleProgress("GuessesRemainingAfterGuessCache#buildCache", reverseIndexLookup.size(), true);

        cache.assign(reverseIndexLookup.size() * NUM_PATTERNS, {});
        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        auto wordIndexes = getVector(reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;
    
        auto lambda = [&]<typename T>(const T& executionType) {
            std::transform(
                executionType,
                wordIndexes.begin(),
                wordIndexes.end(),
                dummy.begin(),
                [
                    &bar,
                    &allPatterns = std::as_const(allPatterns),
                    &wordIndexes = std::as_const(wordIndexes),
                    &reverseIndexLookup = std::as_const(reverseIndexLookup)
                ](const int guessIndex) -> int {
                    bar.incrementAndUpdateStatus();

                    //DEBUG("AttemptStateFaster: " << getPegetIntegerPercrc());
                    for (const auto &pattern: allPatterns) {
                        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);
                        WordSetGuesses ws = {};
                        auto ret = AttemptStateFast::guessWord(guessIndex, wordIndexes, reverseIndexLookup, patternInt);
                        for (auto ind: ret) ws[ind] = true;
                        cache[getIndex(guessIndex, patternInt)] = ws;
                    }
                    return 0;
                }
            );
        };

        if (GlobalArgs.forceSequential) {
            lambda(std::execution::seq);
        } else {
            lambda(std::execution::par_unseq);
        }
    }

    static inline int getIndex(IndexType guessIndex, PatternInt patternInt) {
        return NUM_PATTERNS * guessIndex + patternInt;
    }

    static WordSetGuesses& getFromCacheWithAnswerIndex(IndexType guessIndex, IndexType answerIndex) {
        const auto getter = PatternGetterCached(answerIndex);
        const auto patternInt = getter.getPatternIntCached(guessIndex);
        return getFromCache(guessIndex, patternInt);
    }

    static WordSetGuesses& getFromCache(IndexType guessIndex, PatternInt patternInt) {
        return cache[getIndex(guessIndex, patternInt)];
    }
};
