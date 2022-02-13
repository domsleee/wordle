#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "PatternGetter.h"
#include "Util.h"
#include "WordSetUtil.h"
#include "AttemptStateCacheKey.h"
#include "AttemptStateFastStructs.h"
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <stdexcept>

struct AttemptStateFast {
    AttemptStateFast(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

        if (pattern == "+++++") return {guessIndex};

        std::vector<IndexType> res = {};
        const auto &guessIndexPattern = guessIndexPatternLookup[NUM_PATTERNS * guessIndex + patternInt];
        //DEBUG("PATTERN: " << pattern);
        for (auto wordIndex: wordIndexes) {
            //if (guessIndex == 3117) DEBUG("CHECKING guess: " << wordIndexLookup[guessIndex] << ", word: " << wordIndexLookup[wordIndex]);
            const auto &wordIndexData = wordIndexDataLookup[wordIndex];

            if (wordIndex == guessIndex) continue;

            if (wordIndexData.letterMap & guessIndexPattern.excludedLetterMap != 0) continue;

            if ((wordIndexData.letterCountNumber % guessIndexPattern.letterMinLimitNumber) != 0) continue;
            if ((wordIndexData.positionalLetterNumber % guessIndexPattern.rightSpotNumber) != 0) continue;

            if (std::any_of(
                guessIndexPattern.letterMaxLimit.begin(),
                guessIndexPattern.letterMaxLimit.begin() + guessIndexPattern.letterMaxLimitSize,
                [&](const LetterMaxLimitType &limit) {
                    return (wordIndexData.letterCountNumber % limit) == 0; // divisible if exceeds max
                }
            )) continue;

            //if (gcd(wordIndexData.letterCountNumber, guessIndexPattern.letterMaxLimitNumber) != 1) continue;
            
            if (std::any_of(
                guessIndexPattern.wrongSpotPattern.begin(),
                guessIndexPattern.wrongSpotPattern.begin() + guessIndexPattern.wrongSpotPatternSize,
                [&](const WrongSpotPatternType &wrongSpotPattern) {
                    return (wordIndexData.positionalLetterNumber % wrongSpotPattern) == 0;
                }
            )) continue;
            //if (gcd(wordIndexData.positionalLetterNumber, guessIndexPattern.wrongSpotPatternNumber) != 1) continue;

            res.push_back(wordIndex);
        }

        return res;
        //return guessWord(guessIndex, pattern, wordIndexes, wordIndexLookup);
    }

    int64_t gcd(int64_t a, int64_t b) const {
        while (b != 0) {
            auto tmp = b;
            b = a % b;
            a = tmp;
        }
        return a;
        //return b == 0 ? a : gcd(b, a % b);
    }

    static inline std::vector<uint64_t> firstPrimes = {};
    static inline std::vector<WordIndexData> wordIndexDataLookup = {};
    static inline std::vector<GuessIndexPatternData> guessIndexPatternLookup = {};
    static void buildForReverseIndexLookup(const std::vector<std::string> &reverseIndexLookup) {
        DEBUG("buildForReverseIndexLookup with " << reverseIndexLookup.size() << " strings");
        firstPrimes = getFirstNPrimes<uint64_t>(26 * WORD_LENGTH);
        buildWordIndexDataLookup(reverseIndexLookup);
        buildGuessIndexPatternData(reverseIndexLookup);
        DEBUG("buildForReverseIndexLookup finished");
    }

private:
    static void buildWordIndexDataLookup(const std::vector<std::string> &reverseIndexLookup) {
        DEBUG("build WordIndexData");
        wordIndexDataLookup.clear();
        wordIndexDataLookup.reserve(reverseIndexLookup.size());
        for (std::size_t i = 0; i < reverseIndexLookup.size(); ++i) {
            const auto &word = reverseIndexLookup[i];
            LetterCountNumberType letterCountNumber = 1;
            PositionLetterType positionalLetterNumber = 1;
            int letterMap = 0;
            for (std::size_t j = 0; j < word.size(); ++j) {
                char c = word[j];
                letterMap |= (1 << (c-'a'));
                letterCountNumber = safeMultiply(letterCountNumber, getPrimeForLetter<LetterCountNumberType>(c));
                positionalLetterNumber = safeMultiply(positionalLetterNumber, getPrimeForPosition<PositionLetterType>(c, j));
            }
            wordIndexDataLookup.push_back(WordIndexData(letterCountNumber, positionalLetterNumber, letterMap));
        }
    }

    static void buildGuessIndexPatternData(const std::vector<std::string> &reverseIndexLookup) {
        DEBUG("build GuessIndexPatternData");
        guessIndexPatternLookup.clear();

        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        guessIndexPatternLookup.reserve(NUM_PATTERNS * reverseIndexLookup.size());
        WordAndPatternData data = {};
        for (std::size_t i = 0; i < reverseIndexLookup.size(); ++i) {
            const auto &word = reverseIndexLookup[i];
            for (const auto &pattern: allPatterns) {

                //DEBUG("mc pattern " << pattern);
                calcWordAndPattern(data, word, pattern);

                LetterCountNumberType letterMinLimitNumber = 1;
                PositionLetterType rightSpotNumber = 1;
                std::vector<LetterMaxLimitType> letterMaxLimit = {};
                std::vector<WrongSpotPatternType> wrongSpotPattern = {};
                int excludedLetterMap = 0;

                for (int j = 0; j < WORD_LENGTH; ++j) {
                    if (data.rightSpotPattern[j] != NULL_LETTER) {
                        rightSpotNumber = safeMultiply(rightSpotNumber, getPrimeForPosition<PositionLetterType>(word[j], j));
                    }
                    if (data.wrongSpotPattern[j] != NULL_LETTER) {
                        wrongSpotPattern.push_back(getPrimeForPosition<WrongSpotPatternType>(word[j], j));
                    }
                }

                int knownLetterCount = 0;
                for (int j = 0; j < 26; ++j) {
                    for (auto k = 0; k < data.letterMinLimit[j]; ++k) {
                        letterMinLimitNumber = safeMultiply(letterMinLimitNumber, getPrimeForLetter<LetterCountNumberType>('a' + j));
                        knownLetterCount++;
                    }
                }

                if (knownLetterCount != WORD_LENGTH) {
                    for (int j = 0; j < 26; ++j) {
                        LetterMaxLimitType multForLetter = 1;
                        if (data.letterMaxLimit[j] == MAX_LETTER_LIMIT_MAX) continue;
                        if (data.letterMaxLimit[j] == 0) {
                            excludedLetterMap |= 1 << j;
                            continue;
                        }
                        for (auto k = 0; k <= data.letterMaxLimit[j]; ++k) {
                            multForLetter = safeMultiply(multForLetter, (LetterMaxLimitType)getPrimeForLetter<LetterMaxLimitType>('a' + j));
                        }
                        letterMaxLimit.push_back(multForLetter);
                    }
                }

                //auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

                // NUM_PATTERNS * i + patternInt
                guessIndexPatternLookup.push_back(GuessIndexPatternData(
                    letterMinLimitNumber,
                    rightSpotNumber,
                    letterMaxLimit,
                    wrongSpotPattern,
                    excludedLetterMap
                ));
            }
        }
    }

    static void calcWordAndPattern(WordAndPatternData &data, const std::string &word, const std::string &pattern) {
        for (int i = 0; i < 26; ++i) {
            data.letterMinLimit[i] = 0;
            data.letterMaxLimit[i] = MAX_LETTER_LIMIT_MAX;
        }

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            if (pattern[i] == '+' || pattern[i] == '?') {
                auto ind = word[i]-'a';
                data.letterMinLimit[ind]++;
            }
        }

        for (std::size_t i = 0; i < pattern.size(); ++i) {
            data.rightSpotPattern[i] = (pattern[i] == '+' ? word[i] : NULL_LETTER);
            data.wrongSpotPattern[i] = (pattern[i] == '?' ? word[i] : NULL_LETTER);

            int letterInd = word[i]-'a';
            if (pattern[i] == '_') {
                if (data.letterMinLimit[letterInd] == 0) {
                    data.letterMaxLimit[letterInd] = 0;
                } else {
                    data.letterMaxLimit[letterInd] = data.letterMinLimit[letterInd];
                }
            }
        }
    }

    template <typename T>
    static T getPrimeForLetter(char c) {
        return (T)firstPrimes[c-'a'];
    }

    template <typename T>
    static T getPrimeForPosition(char c, int position) {
        int charIndex = c-'a';
        uint64_t ind = 26 * position + charIndex;
        if (ind > firstPrimes.size()) {
            if (ind < 0) {
                DEBUG("HELLO");
            }
            DEBUG("requested a large prime " << (int64_t)ind << ", total primes: " << firstPrimes.size());
            exit(1);
        }
        uint64_t v = firstPrimes[ind];
        T M = std::numeric_limits<T>::max();
        if (v > M) {
            DEBUG("cannot cast! trying: " << (int64_t)v << " to " << (int64_t)M << " typeid: " << typeid(v).name());
            //throw std::runtime_error("fail");
            exit(1);
        }
        return (T)v;
    }

    template <typename T>
    static std::vector<T> getFirstNPrimes(int n) {
        std::vector<T> res(n);
        T resIndex = 0;
        for (T i = 2; resIndex < n; ++i) {
            bool valid = true;
            auto sqrti = (int)sqrt(i);
            for (T j = 0; j < resIndex && res[j] <= sqrti; ++j) {
                if (i % res[j] == 0) { valid = false; break; }
            }
            if (valid) res[resIndex++] = i;
        }
        return res;
    }
};