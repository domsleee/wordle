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
#include "BigBitsetProxy.h"

#include "Util.h"
#define ATTEMPTSTATEFASTER_DEBUG(x) DEBUG(x)

struct AttemptStateFaster {
    AttemptStateFaster(const PatternGetter &getter)
      : patternGetter(getter) {}

    PatternGetter patternGetter;

    std::vector<IndexType> guessWord(IndexType guessIndex, const std::vector<IndexType> &wordIndexes, const std::vector<std::string> &wordIndexLookup) const {
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

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
        auto pattern = patternGetter.getPatternFromWord(wordIndexLookup[guessIndex]);
        auto patternInt = AttemptStateCacheKey::calcPatternInt(pattern);

        // is equal to +++++
        if (patternInt == NUM_PATTERNS-1) {
            auto sizeBefore = wordIndexes.size();
            for (std::size_t i = 0; i < wordIndexes.size(); ++i) {
                auto wordIndex = wordIndexes[i];
                if (wordIndex != guessIndex) {
                    wordIndexes.deleteIndex(i);
                    i--;
                }
            }

            return sizeBefore-1;
        }

        const auto &ws = cache[NUM_PATTERNS * guessIndex + patternInt];
        std::size_t removed = 0;
        auto sizeBefore = wordIndexes.size();
        for (std::size_t i = 0; i < wordIndexes.size(); ++i) {
            auto wordIndex = wordIndexes[i];
            if (ws[wordIndex]) {
                wordIndexes.deleteIndex(i);
                removed++;
                i--;
            }
        }
        return removed;
    }

    static inline std::vector<BigBitset> cache;
    static void buildWSLookup(const std::vector<std::string> &reverseIndexLookup) {
        ATTEMPTSTATEFASTER_DEBUG("buildWSLookup");
        cache.assign(reverseIndexLookup.size() * NUM_PATTERNS, {});
        auto allPatterns = AttemptState::getAllPatterns(WORD_LENGTH);
        auto wordIndexes = getVector(reverseIndexLookup.size(), 0);
        auto dummy = wordIndexes;
        std::atomic<int> done = 0;

        std::string filename = "databases/test.out";

        if (false && std::filesystem::exists(filename)) {
            DEBUG("reading from file...");
            auto v = MyProxy(cache);
            AttemptStateUtil::readFromFile(v, (int64_t)cache.size() * REVERSE_INDEX_LOOKUP_SIZE, filename);
            return;
        }

        std::transform(
            std::execution::par,
            wordIndexes.begin(),
            wordIndexes.end(),
            dummy.begin(),
            [
                &cache,
                &done,
                &allPatterns = std::as_const(allPatterns),
                &wordIndexes = std::as_const(wordIndexes),
                &reverseIndexLookup = std::as_const(reverseIndexLookup)
            ](const int guessIndex) -> int {
                done++;
                DEBUG("AttemptStateFaster: " << getPerc(done.load(), reverseIndexLookup.size()));
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
        
        if (false) AttemptStateUtil::writeToFile(MyProxy(cache), (int64_t)cache.size() * REVERSE_INDEX_LOOKUP_SIZE, filename);
        if (false) {
            std::vector<BigBitset> cache2;
            cache2.assign(reverseIndexLookup.size() * NUM_PATTERNS, {});
            auto v = MyProxy(cache2);
            AttemptStateUtil::readFromFile(v, (int64_t)cache.size() * REVERSE_INDEX_LOOKUP_SIZE, filename);
            for (std::size_t i = 0; i < cache.size(); ++i) {
                if (cache[i] != cache2[i]) {
                    DEBUG("MISMATCH AT " << i);
                    exit(1);
                }
            }

        }
        ATTEMPTSTATEFASTER_DEBUG("buidWSLookup finished");
    }

};