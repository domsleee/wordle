#pragma once
#include <vector>
#include <algorithm>
#include <execution>
#include "Defs.h"
#include "WordSetUtil.h"
#include "WordSetHelpers.h"
#include "GlobalArgs.h"

#include "SimpleProgress.h"

// the possible guesses that remain after guessing a word and receiving a pattern
struct GuessesRemainingAfterGuessCache {
    static inline std::vector<WordSetGuesses> cache;
    static void buildCache() {
        auto bar = SimpleProgress("GuessesRemainingAfterGuessCache#buildCache", GlobalState.reverseIndexLookup.size(), true);

        cache.assign(GlobalState.reverseIndexLookup.size() * NUM_PATTERNS, {});
        auto allPatternInts = getVector(NUM_PATTERNS, 0);
        auto wordIndexes = getVector(GlobalState.reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;
    
        auto lambda = [&]<typename T>(const T& executionType) {
            std::transform(
                executionType,
                wordIndexes.begin(),
                wordIndexes.end(),
                dummy.begin(),
                [
                    &bar,
                    &allPatternInts = std::as_const(allPatternInts),
                    &wordIndexes = std::as_const(wordIndexes),
                    &reverseIndexLookup = std::as_const(GlobalState.reverseIndexLookup)
                ](const int guessIndex) -> int {
                    bar.incrementAndUpdateStatus();

                    std::array<bool, NUM_PATTERNS> possiblePattern = {};
                    for (std::size_t i = 0; i < NUM_PATTERNS; ++i) possiblePattern[i] = false;
                    for (auto actualWordIndex: wordIndexes) {
                        const auto getter = PatternGetterCached(actualWordIndex);
                        auto patternInt = getter.getPatternIntCached(guessIndex);
                        possiblePattern[patternInt] = true;
                    }

                    //DEBUG("AttemptStateFaster: " << getPegetIntegerPercrc());
                    for (const auto &patternInt: allPatternInts) {
                        if (!possiblePattern[patternInt]) continue;
                        cache[getIndex(guessIndex, patternInt)] = guessWordWordSet(guessIndex, wordIndexes, reverseIndexLookup, patternInt);
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

    static inline int getIndex(IndexType guessIndex, PatternType patternInt) {
        return NUM_PATTERNS * guessIndex + patternInt;
    }

    static WordSetGuesses& getFromCacheWithAnswerIndex(IndexType guessIndex, IndexType answerIndex) {
        const auto getter = PatternGetterCached(answerIndex);
        const auto patternInt = getter.getPatternIntCached(guessIndex);
        return getFromCache(guessIndex, patternInt);
    }

    static WordSetGuesses& getFromCache(IndexType guessIndex, PatternType patternInt) {
        return cache[getIndex(guessIndex, patternInt)];
    }

    static WordSetGuesses guessWordWordSet(
        IndexType guessIndex,
        const std::vector<IndexType> &wordIndexes,
        const std::vector<std::string> &reverseIndexLookup,
        PatternType patternInt)
    {        
        /*const auto patternStr = PatternIntHelpers::patternIntToString(patternInt);
        const auto guessesVec = AttemptState::guessWord(guessIndex, patternStr, wordIndexes, reverseIndexLookup);
        return WordSetHelpers::buildGuessesWordSet(guessesVec);*/
        
        return AttemptState::filterWordsMatchingGuessPattern(guessIndex, patternInt, wordIndexes);
    }
};