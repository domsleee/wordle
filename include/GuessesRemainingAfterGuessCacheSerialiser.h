#pragma once
#include <string>
#include <vector>
#include "WordSetUtil.h"
#include "GuessesRemainingAfterGuessCache.h"

struct GuessesRemainingAfterGuessCacheSerialiser {
    static void readFromFile(const std::string &file) {
        // todo.
    }

    static void writeToFile(const std::string &file) {
        START_TIMER(writeToFile);
        auto num8BytePieces = (WordSetGuesses().size() + 63) / 64;
        auto &cache = GuessesRemainingAfterGuessCache::cache;
        std::vector<uint64_t> tempVal(num8BytePieces * cache.size(), 0);

        for (std::size_t i = 0; i < cache.size(); ++i) {
            std::size_t offset = i*num8BytePieces;
            int bitIndex = 0, my8ByteIndex = 0;
            for (std::size_t j = 0; j < cache[i].size(); ++j) {
                if (cache[i][j]) {
                    tempVal[offset + my8ByteIndex] &= 0b1 << bitIndex;
                }
                bitIndex = (bitIndex + 1) & 0b111111;
                my8ByteIndex += (bitIndex == 0);
            }
        }

        // done.
        END_TIMER(writeToFile);
    }
};