#pragma once
#include "PerfStats.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesUsingNonLetterMask.h"
#include "SimpleProgress.h"
#include "Util.h"
#include "WordSetHelpers.h"

#include <filesystem>
#include <unordered_map>

struct RemoveGuessesWithBetterGuessCache
{
    static inline std::unordered_map<int, GuessesVec> cache = {};
    // static inline std::unordered_map<int, WordSetGuesses> cacheWsGuesses = {};

    static void init()
    {
        cache = {};

        auto filename = FROM_SS("databases/betterGuess"
            << "_" << GlobalArgs.specialLetters
            << "_" << getFilenameIdentifier()
            << ".dat");
        if (std::filesystem::exists(filename)) {
            readFromFile(filename);
            // checkCompression();
            return;
        }

        START_TIMER(removeguessesbetterguesscache);
        auto expected = 1 << (GlobalArgs.specialLetters.size());
        auto bar = SimpleProgress("RemoveGuessesWithBetterGuessCache", expected, true);

        int max = (1 << 26);
        auto stats = PerfStats();
        const auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        long long totalSize = 0;
        for (int i = 0; i < max; ++i)
        {
            int nonLetterMask = i & RemoveGuessesUsingNonLetterMask::specialMask;
            if (cache.find(nonLetterMask) != cache.end()) continue;
            auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
            auto guessesFast = guesses;
            const bool compareWithSlowMethod = false;
            if (compareWithSlowMethod) RemoveGuessesUsingNonLetterMask(stats, nonLetterMask).removeWithBetterOrSameGuessFast(guesses);
            RemoveGuessesUsingNonLetterMask(stats, nonLetterMask).removeWithBetterOrSameGuessFaster(guessesFast);

            if (compareWithSlowMethod && guesses.size() != guessesFast.size()) {
                DEBUG("OH NO " << guesses.size() << " VS " << guessesFast.size()); exit(1);
            }

            totalSize += guessesFast.size();
            cache[nonLetterMask] = guessesFast;
            // cacheWsGuesses[nonLetterMask] = WordSetHelpers::buildGuessesWordSet(guessesFast);
            if (cache.size()%100 == 0) bar.incrementAndUpdateStatus("", 100);
        }
        bar.dispose();
        DEBUG("Generated RemoveGuessesWithBetterGuessCache: " << filename);
        DEBUG("cache size: " << cache.size() << " totalSize: " << totalSize);
        if (GlobalArgs.timings) {
            stats.printEntry("removeSubs", 44);
            stats.printEntry("10", 10);
            stats.printEntry("11", 11);
            stats.printEntry("12", 12);
            stats.printEntry("13", 13);
        }

        END_TIMER(removeguessesbetterguesscache);

        writeToFile(filename);
    }

    static const GuessesVec &getCachedGuessesVec(const AnswersVec &answers)
    {
        return cache[getNonLetterMask(answers)];
    }

    // static const WordSetGuesses &getCachedGuessesWordSet(const AnswersVec &answers)
    // {
    //     return cacheWsGuesses[getNonLetterMask(answers)];
    // }

    // static GuessesVec buildGuessesVecWithoutRemovingAnyAnswer(const GuessesVec &guesses, const AnswersVec &answers, int nonLetterMask)
    // {
    //     GuessesVec vec = guesses;
    //     auto wsAnswers = WordSetHelpers::buildAnswersWordSet(answers);
    //     auto &wsGoodGuesses = cacheWsGuesses[nonLetterMask];
    //     std::erase_if(vec, [&](const auto guessIndex) -> bool {
    //         auto isAnAnswer = guessIndex < GlobalState.allAnswers.size() && wsAnswers[guessIndex];
    //         return !wsGoodGuesses[guessIndex] && !isAnAnswer;
    //     });
    //     return vec;
    // }

    static int getNonLetterMask(const AnswersVec &answers)
    {
        return getNonLetterMaskNoSpecialMask(answers) & RemoveGuessesUsingNonLetterMask::specialMask;
    }

    static int getNonLetterMaskNoSpecialMask(const AnswersVec &answers) {
        int answerLetterMask = 0;
        for (auto answerIndex : answers)
        {
            answerLetterMask |= RemoveGuessesUsingNonLetterMask::letterCountLookup[answerIndex];
        }
        int nonLetterMask = ~answerLetterMask;
        return nonLetterMask;
    }

    static void readFromFile(const std::string &filename) {
        START_TIMER(betterGuessCacheReadFromFile);
        long long totalSize = 0;
        std::ifstream fin(filename, std::ios_base::binary);
        int cacheSize;
        fin.read(reinterpret_cast<char*>(&cacheSize), sizeof(cacheSize));
        
        int index, numEntries;
        while (cacheSize--) {
            fin.read(reinterpret_cast<char*>(&index), sizeof(index));
            fin.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
            cache[index].resize(numEntries);
            fin.read(reinterpret_cast<char*>(cache[index].data()), sizeof(IndexType) * numEntries);
            //cacheWsGuesses[index] = WordSetHelpers::buildGuessesWordSet(cache[index]);
            totalSize += numEntries;
        }
        fin.close();
        DEBUG("Loaded RemoveGuessesWithBetterGuessCache: " << filename);
        DEBUG("cache size: " << cache.size() << " totalSize: " << totalSize);

        END_TIMER(betterGuessCacheReadFromFile);
    }

    static void writeToFile(const std::string &filename) {
        START_TIMER(betterGuessCacheWriteToFile);
        std::ofstream fout(filename, std::ios_base::binary);
        int cacheSize = cache.size();
        fout.write(reinterpret_cast<char*>(&cacheSize), sizeof(cacheSize));
        for (auto &entry: cache) {
            int index = entry.first, numEntries = (int)entry.second.size();
            fout.write(reinterpret_cast<char*>(&index), sizeof(index));
            fout.write(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
            fout.write(reinterpret_cast<char*>(entry.second.data()), sizeof(IndexType) * numEntries);
        }
        fout.close();
        END_TIMER(betterGuessCacheWriteToFile);
    }

    // static void checkCompression() {
    //     std::unordered_set<WordSetGuesses> sets = {};
    //     long long totalCt = 0, setCt = 0;
    //     for (auto &entry: cacheWsGuesses) {
    //         sets.insert(entry.second);
    //         totalCt += entry.second.count();
    //     }
    //     for (auto &entry: sets) setCt += entry.count();
    //     DEBUG("Compressable? " << getPerc(setCt, totalCt));
    //     exit(1);
    // }
};
