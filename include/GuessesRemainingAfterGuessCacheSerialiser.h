#pragma once
#include <string>
#include <vector>
#include "WordSetUtil.h"
#include "GuessesRemainingAfterGuessCache.h"

struct GuessesRemainingAfterGuessCacheSerialiser {
    static void readFromFile(const std::string &file) {
        // todo.
    }

    static std::vector<uint64_t> writeToFile(const std::string &file) {
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
        return tempVal;
    }

    static void copy() {
        START_TIMER(copy);
        auto tempVal = writeToFile("SDF");
        std::vector<WordSetGuesses> myCache;
        myCache.assign(GuessesRemainingAfterGuessCache::cache.size(), {});
        auto num8BytePieces = (WordSetGuesses().size() + 63) / 64;

        int cacheI = 0, bitOffset = 0;
        for (std::size_t i = 0; i < tempVal.size(); ++i) {
            for (std::size_t j = 0; j < 64; ++j) {
                if (tempVal[i] & (0b1 << j)) {
                    myCache[cacheI][bitOffset] = true;
                }
            }
            if (i > 0 && i % num8BytePieces == 0) {
                cacheI++; bitOffset = 0;
            } else {
                bitOffset += 64;
            }
        }
        END_TIMER(copy);
    }
};