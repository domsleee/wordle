#pragma once
#include <string>
#include <vector>
#include <cstring>
#include <filesystem>
#include "WordSetUtil.h"
#include "GuessesRemainingAfterGuessCache.h"
#include  <fcntl.h>
#include <unistd.h>

struct GuessesRemainingAfterGuessCacheSerialiser {
    static void initialiseFromCache() {
        const auto filename = "databases/"
         + getAfterSlash(GlobalArgs.guesses)
         + "_"
         + getAfterSlash(GlobalArgs.answers)
         + "_"
         + std::to_string(WordSetGuesses().size())
         + "_"
         + std::to_string(GuessesRemainingAfterGuessCache::getCacheSize());
        if (std::filesystem::exists(filename)) {
            DEBUG("readFromFile " << filename);
            readFromFile(filename);
            return;
        }

        GuessesRemainingAfterGuessCache::buildCache();
        DEBUG("write to file: \"" << filename << '"');
        writeToFile(filename);
    }

private:
    static std::string getAfterSlash(const std::string &filename) {
        return filename.substr(filename.find_last_of("/")+1);
    }

    static void readFromFile(const std::string &filename) {
        START_TIMER(readFromFile);
        auto &cache = GuessesRemainingAfterGuessCache::cache;
        cache.assign(GuessesRemainingAfterGuessCache::getCacheSize(), {});

        FILE *fptr;
        if ((fptr = fopen(filename.c_str(), "rb")) == NULL){
            DEBUG("Error! opening file");
            exit(1);
        }
        auto bytesRead = fread((void*)cache.data(), getSizeInBytes(), 1, fptr);
        DEBUG("bytes read? " << bytesRead);
        fclose(fptr);

        /*
        std::ifstream ifs(filename, std::ios::binary);
        GuessesRemainingAfterGuessCache::cache.assign(GuessesRemainingAfterGuessCache::getCacheSize(), {});
        ifs.read(reinterpret_cast<char*>(GuessesRemainingAfterGuessCache::cache.data()), getSizeInBytes());
        ifs.close();
        */
       END_TIMER(readFromFile);
    }

    static void writeToFile(const std::string &filename) {
        START_TIMER(writeToFile);
        const auto &cache = GuessesRemainingAfterGuessCache::cache;

        /*
        std::ofstream of(filename, std::ios::binary);
        if (!of.is_open()) {
            DEBUG("failed to open..");
        }
        DEBUG("write this many bytes... " << getSizeInBytes());
        DEBUG("cache size... " << cache.size());
        if (getSizeInBytes() > std::numeric_limits<std::streamsize>::max()) {
            DEBUG("larger than max"); exit(1);
        }
        of.write(reinterpret_cast<const char*>(cache.data()), getSizeInBytes());
        of.close();
        if (!of.good()) {
            DEBUG("no good ofstream after writing..");
        }
        */
        FILE *fptr;
        if ((fptr = fopen(filename.c_str(), "wb")) == NULL){
            DEBUG("Error! opening file");
            exit(1);
        }
        fwrite((void*)cache.data(), getSizeInBytes(), 1, fptr);
        fclose(fptr);

        END_TIMER(writeToFile);
    }

    static std::size_t getSizeInBytes() {
        const auto num8BytePieces = (WordSetGuesses().size() + 63) / 64;
        const auto cacheSize = GuessesRemainingAfterGuessCache::getCacheSize();
        return cacheSize * num8BytePieces * 8;
    }
};