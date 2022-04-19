#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <stdexcept>

#include "AttemptState.h"
#include "PatternGetter.h"
#include "Util.h"
#include "Defs.h"
#include "WordSetUtil.h"
#include "AttemptStateCacheKey.h"
#include "AttemptStateFastStructs.h"

#define ATTEMPTSTATEFAST_DEBUG(x) 

struct AttemptStateFast {
    AttemptStateFast(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    inline static long long guessWordCt = 0;
    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        //guessWordCt++;
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) return {guessIndex};
        return guessWord(guessIndex, wordIndexes, wordIndexLookup, patternInt);
    }

    static std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup, int patternInt) {

        /*
        auto otherResult = AttemptState(patternGetter).guessWord(guessIndex, wordIndexes, wordIndexLookup);
        DEBUG("guess index " << guessIndex << " other " << otherResult.size() << ", pattern " << pattern);
        DEBUG("other: " << otherResult[0]);
        */

        std::vector<IndexType> res = {};
        res.reserve(wordIndexes.size());
        const auto &guessIndexPattern = guessIndexPatternLookup[NUM_PATTERNS * guessIndex + patternInt];
        for (auto wordIndex: wordIndexes) {
            /*if (guessIndex == 2315 && wordIndex == 1 && pattern == "____?") {
                DEBUG("CHECKING guess: " << wordIndexLookup[guessIndex] << ", word: " << wordIndexLookup[wordIndex] << ", pattern: " << pattern << ", " << otherResult.size() << " : " << otherResult[0]);
            }*/
            if (wordIndex == guessIndex) continue;

            const auto &wordIndexData = wordIndexDataLookup[wordIndex];

            if ((wordIndexData.letterMap & guessIndexPattern.excludedLetterMap) != 0) continue;

            // replaced by one 32-bit bitwise AND
            if ((wordIndexData.positionalLetterNumber & guessIndexPattern.rightSpotNumber.mask) != guessIndexPattern.rightSpotNumber.value) continue;

            // replaced by two 64-bit bitwise AND. Can't do more because it needs to ensure it is >= min
            //if ((wordIndexData.letterCountNumber % guessIndexPattern.letterMinLimitNumber) != 0) continue;
            
            const auto &v1 = readAsInt64(guessIndexPattern.letterMinLimit, 0);
            if ((readAsInt64(wordIndexData.letterCountMap, 0) & v1) != v1) continue;
            const auto &v2 = readAsInt64(guessIndexPattern.letterMinLimit, 1);
            if ((readAsInt64(wordIndexData.letterCountMap, 1) & v2) != v2) continue;

            if (std::any_of(
                guessIndexPattern.letterMaxLimit.begin(),
                guessIndexPattern.letterMaxLimit.begin() + guessIndexPattern.letterMaxLimitSize,
                [&](const LetterToCheckLetterMap &pair) {
                    return (wordIndexData.letterCountMap[pair.letterCountToCheck] & pair.letterMap) != 0; // divisible if exceeds max
                }
            )) continue;

            //if (gcd(wordIndexData.letterCountNumber, guessIndexPattern.letterMaxLimitNumber) != 1) continue;
            
            // replaced by 32-bit bitwise ANDs
            if (std::any_of(
                guessIndexPattern.wrongSpotPattern.begin(),
                guessIndexPattern.wrongSpotPattern.begin() + guessIndexPattern.wrongSpotPatternSize,
                [&](const auto &wrongSpotPattern) {
                    return (wordIndexData.positionalLetterNumber & wrongSpotPattern.mask) == wrongSpotPattern.value;
                }
            )) continue;
            //if (gcd(wordIndexData.positionalLetterNumber, guessIndexPattern.wrongSpotPatternNumber) != 1) continue;

            res.push_back(wordIndex);
        }

        return res;
        //return guessWord(guessIndex, pattern, wordIndexes, wordIndexLookup);
    }

    static inline int64_t readAsInt64(const std::array<LetterMapType, 4> &arr, int index) {
        return reinterpret_cast<const std::array<int64_t, 2>&>(arr)[index];
    }

    // https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op
    template <typename T>
    T fastMod(const T input, const T ceil) const {
        // apply the modulo operator only when needed
        // (i.e. when the input is greater than the ceiling)
        return input >= ceil ? input % ceil : input;
        // NB: the assumption here is that the numbers are positive
    }

    static int64_t gcd(int64_t a, int64_t b) {
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
        ATTEMPTSTATEFAST_DEBUG("buildForReverseIndexLookup with " << reverseIndexLookup.size() << " strings");
        firstPrimes = getFirstNPrimes<uint64_t>(26 * WORD_LENGTH);
        buildWordIndexDataLookup(reverseIndexLookup);
        buildGuessIndexPatternData(reverseIndexLookup);
        ATTEMPTSTATEFAST_DEBUG("buildForReverseIndexLookup finished");
    }

    static void clearCache() {
        firstPrimes.clear();
        wordIndexDataLookup.clear();
        guessIndexPatternLookup.clear();
        firstPrimes.shrink_to_fit();
        wordIndexDataLookup.shrink_to_fit();
        guessIndexPatternLookup.shrink_to_fit();
    }

private:
    static void buildWordIndexDataLookup(const std::vector<std::string> &reverseIndexLookup) {
        ATTEMPTSTATEFAST_DEBUG("build WordIndexData");
        wordIndexDataLookup.clear();
        wordIndexDataLookup.reserve(reverseIndexLookup.size());
        for (std::size_t i = 0; i < reverseIndexLookup.size(); ++i) {
            const auto &word = reverseIndexLookup[i];
            LetterCountNumberType letterCountNumber = 1;
            PositionLetterType positionalLetterNumber = 0;
            MinLetterType letterCount = {};
            int letterMap = 0;
            for (std::size_t j = 0; j < word.size(); ++j) {
                char c = word[j];
                letterMap |= (1 << (c-'a'));
                letterCountNumber = safeMultiply(letterCountNumber, getPrimeForLetter<LetterCountNumberType>(c));
                positionalLetterNumber |= (c-'a') << (5 * j);
                letterCount[c-'a']++;
            }
            std::array<LetterMapType, 4> letterCountMap = {};
            for (int j = 1; j <= 4; ++j) {
                for (int k = 0; k < 26; ++k) {
                    letterCountMap[j-1] |= ((letterCount[k] >= j) << k);
                }
            }

            wordIndexDataLookup.push_back(WordIndexData(
                letterCountNumber,
                positionalLetterNumber,
                letterMap,
                letterCountMap));
        }
    }

    static void buildGuessIndexPatternData(const std::vector<std::string> &reverseIndexLookup) {
        ATTEMPTSTATEFAST_DEBUG("build GuessIndexPatternData");
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
                std::array<LetterMapType, 4> letterMinLimit = {};
                PositionLetterType rightSpotNumber = 0, rightSpotMask = 0;
                std::vector<LetterToCheckLetterMap> letterMaxLimit = {};
                std::vector<ValueWithMask<PositionLetterType>> wrongSpotPattern = {};
                LetterMapType excludedLetterMap = 0;

                for (int j = 0; j < WORD_LENGTH; ++j) {
                    if (data.rightSpotPattern[j] != NULL_LETTER) {
                        rightSpotNumber |= (word[j]-'a') << (5 * j);
                        rightSpotMask |= (0b11111 << (5 * j));
                    }
                    if (data.wrongSpotPattern[j] != NULL_LETTER) {
                        wrongSpotPattern.push_back(
                            ValueWithMask<PositionLetterType>(
                                (word[j]-'a') << (5 * j),
                                (0b11111 << (5 * j))
                            )
                        );
                    }
                }

                int knownLetterCount = 0;
                for (int j = 0; j < 26; ++j) {
                    if (data.letterMinLimit[j] == 0) continue;
                    for (auto k = 0; k < data.letterMinLimit[j]; ++k) {
                        letterMinLimitNumber = safeMultiply(letterMinLimitNumber, getPrimeForLetter<LetterCountNumberType>('a' + j));
                        knownLetterCount++;
                    }

                    letterMinLimit[data.letterMinLimit[j]-1] |= 1 << j;
                }

                if (knownLetterCount != WORD_LENGTH) {
                    for (int j = 0; j < 26; ++j) {
                        if (data.letterMaxLimit[j] == MAX_LETTER_LIMIT_MAX) continue;
                        if (data.letterMaxLimit[j] == 0) {
                            excludedLetterMap |= 1 << j;
                            continue;
                        }
                        if (data.letterMaxLimit[j] == WORD_LENGTH-1) continue; // assumption "aaaa" is not a word
                        auto newEntry = LetterToCheckLetterMap(data.letterMaxLimit[j], 0b1 << j);
                        auto it = std::find_if(letterMaxLimit.begin(), letterMaxLimit.end(), [&](const LetterToCheckLetterMap &entry) {
                            return entry.letterCountToCheck == newEntry.letterCountToCheck;
                        });
                        if (it != letterMaxLimit.end()) {
                            it->letterMap |= newEntry.letterMap;
                        } else {
                            letterMaxLimit.push_back(newEntry);
                        }
                    }
                }

                //auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

                // NUM_PATTERNS * i + patternInt
                guessIndexPatternLookup.push_back(GuessIndexPatternData(
                    letterMinLimitNumber,
                    letterMinLimit,
                    ValueWithMask<PositionLetterType>(rightSpotNumber, rightSpotMask),
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
    static std::vector<T> getFirstNPrimes(T n) {
        std::vector<T> res(n);
        T resIndex = 0;
        for (T i = 2; resIndex < n; ++i) {
            bool valid = true;
            auto sqrti = (T)sqrt(i);
            for (T j = 0; j < resIndex && res[j] <= sqrti; ++j) {
                if (i % res[j] == 0) { valid = false; break; }
            }
            if (valid) res[resIndex++] = i;
        }
        return res;
    }
};