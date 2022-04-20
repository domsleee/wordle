#pragma once
#include <bitset>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <execution>
#include "AttemptStateFast.h"
#include "AttemptStateUtil.h"
#include "UnorderedVector.h"
#include "PatternGetterCached.h"
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "../third_party/indicators/cursor_control.hpp"
#include "../third_party/indicators/progress_bar.hpp"

#define ATTEMPTSTATEFASTER_DEBUG(x) DEBUG(x)

struct AttemptStateFaster {
    AttemptStateFaster(const PatternGetterCached &getter)
      : patternGetter(getter) {}

    const PatternGetterCached patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto patternInt = patternGetter.getPatternIntCached(guessIndex);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) return {guessIndex};

        std::vector<IndexType> res(wordIndexes.size());
        const auto &ws = cache[NUM_PATTERNS * guessIndex + patternInt];
        int i = 0;
        for (auto wordIndex: wordIndexes) {
            if (ws[wordIndex]) res[i++] = wordIndex;
        }
        res.resize(i);

        return res;
    }

    std::size_t guessWordAndRemoveUnmatched(IndexType guessIndex, UnorderedVector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto patternInt = patternGetter.getPatternIntCached(guessIndex);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) {
            std::size_t removed = 0;
            for (std::size_t i = wordIndexes.size()-1; i != MAX_SIZE_VAL; --i) {
                auto wordIndex = wordIndexes[i];
                if (wordIndex != guessIndex) {
                    wordIndexes.deleteIndex(i);
                    removed++;
                }
            }

            return removed;
        }

        const auto &ws = cache[NUM_PATTERNS * guessIndex + patternInt];
        std::size_t removed = 0;
        for (std::size_t i = wordIndexes.size()-1; i != MAX_SIZE_VAL; --i) {
            auto wordIndex = wordIndexes[i];
            if (!ws[wordIndex]) {
                //DEBUG("INSPECTING " << i << ", wordIndex: " << wordIndex << ", size: " << wordIndexes.size());
                wordIndexes.deleteIndex(i);
                removed++;
            }
        }
        return removed;
    }

    static inline std::vector<BigBitset> cache;
    static void buildWSLookup(const std::vector<std::string> &reverseIndexLookup) {
        indicators::ProgressBar bar{
            indicators::option::BarWidth{30},
            indicators::option::Start{"["},
            indicators::option::Fill{"■"},
            indicators::option::Lead{"■"},
            indicators::option::Remainder{"-"},
            indicators::option::End{" ]"},
            indicators::option::ForegroundColor{indicators::Color::cyan},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::ShowRemainingTime{true},
        };
        
        //ATTEMPTSTATEFASTER_DEBUG("buildWSLookup");
        cache.assign(reverseIndexLookup.size() * NUM_PATTERNS, {});
        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        auto wordIndexes = getVector(reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;
        std::atomic<int> done = 0;
    
        auto lambda = [&]<typename T>(const T& executionType) {
            std::transform(
                executionType,
                wordIndexes.begin(),
                wordIndexes.end(),
                dummy.begin(),
                [
                    &done,
                    &bar,
                    &allPatterns = std::as_const(allPatterns),
                    &wordIndexes = std::as_const(wordIndexes),
                    &reverseIndexLookup = std::as_const(reverseIndexLookup)
                ](const int guessIndex) -> int {
                    done++;
                    bar.set_progress(getIntegerPerc(done.load(), reverseIndexLookup.size()));
                    bar.set_option(indicators::option::PostfixText{"AttemptStateFaster#buildWSLookup: " + getFrac(done.load() + 1, reverseIndexLookup.size())});  

                    //DEBUG("AttemptStateFaster: " << getPegetIntegerPercrc());
                    for (const auto &pattern: allPatterns) {
                        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);
                        BigBitset ws = {};
                        auto ret = AttemptStateFast::guessWord(guessIndex, wordIndexes, reverseIndexLookup, patternInt);
                        for (auto ind: ret) ws[ind] = true;
                        cache[guessIndex * NUM_PATTERNS + patternInt] = ws;
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

        ATTEMPTSTATEFASTER_DEBUG("AttemptStateFaster#buildWSLookup finished");
    }

};