#pragma once
#include "RemoveGuessesWithNoLetterInAnswers.h"
#include "WordSetHelpers.h"
#include "PerfStats.h"
#include "SimpleProgress.h"
#include <filesystem>

#include "Util.h"
struct RemoveGuessesWithBetterGuessCache
{
    static inline std::unordered_map<int, GuessesVec> cache = {};
    static inline std::unordered_map<int, WordSetGuesses> cacheWsGuesses = {};

    static void init()
    {
        auto filename = FROM_SS("databases/betterGuess"
            << "_" << GlobalState.allAnswers.size()
            << "_" << GlobalState.allGuesses.size()
            << "_" << GlobalArgs.specialLetters
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
            int nonLetterMask = i & RemoveGuessesWithNoLetterInAnswers::specialMask;
            if (cache.find(nonLetterMask) != cache.end()) continue;
            auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
            auto guessesFast = guesses;
            const bool compareWithSlowMethod = false;
            if (compareWithSlowMethod) RemoveGuessesWithNoLetterInAnswers::removeWithBetterOrSameGuessFast(stats, guesses, nonLetterMask);
            RemoveGuessesWithNoLetterInAnswers::removeWithBetterOrSameGuessFaster(stats, guessesFast, nonLetterMask);

            if (compareWithSlowMethod && guesses.size() != guessesFast.size()) {
                DEBUG("OH NO " << guesses.size() << " VS " << guessesFast.size()); exit(1);
            }

            totalSize += guessesFast.size();
            cache[nonLetterMask] = guessesFast;
            cacheWsGuesses[nonLetterMask] = WordSetHelpers::buildGuessesWordSet(guessesFast);
            if (cacheWsGuesses.size()%100 == 0) bar.incrementAndUpdateStatus("", 100);
        }
        bar.dispose();
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

    static const WordSetGuesses &getCachedGuessesWordSet(const AnswersVec &answers)
    {
        return cacheWsGuesses[getNonLetterMask(answers)];
    }

    static GuessesVec buildGuessesVecWithoutRemovingAnyAnswer(const GuessesVec &guesses, const AnswersVec &answers, int nonLetterMask)
    {
        GuessesVec vec = guesses;
        auto wsAnswers = WordSetHelpers::buildAnswersWordSet(answers);
        auto &wsGoodGuesses = cacheWsGuesses[nonLetterMask];
        std::erase_if(vec, [&](const auto guessIndex) -> bool {
            auto isAnAnswer = guessIndex < GlobalState.allAnswers.size() && wsAnswers[guessIndex];
            return !wsGoodGuesses[guessIndex] && !isAnAnswer;
        });
        return vec;
    }

    static int getNonLetterMask(const AnswersVec &answers)
    {
        return getNonLetterMaskNoSpecialMask(answers) & RemoveGuessesWithNoLetterInAnswers::specialMask;
    }

    static int getNonLetterMaskNoSpecialMask(const AnswersVec &answers) {
        int answerLetterMask = 0;
        for (auto answerIndex : answers)
        {
            answerLetterMask |= RemoveGuessesWithNoLetterInAnswers::letterCountLookup[answerIndex];
        }
        int nonLetterMask = ~answerLetterMask;
        return nonLetterMask;
    }

    static void readFromFile(const std::string &filename) {
        START_TIMER(betterGuessCacheReadFromFile);
        std::ifstream fin(filename, std::ios_base::binary);
        int cacheSize;
        fin.read(reinterpret_cast<char*>(&cacheSize), sizeof(cacheSize));
        
        int index, numEntries;
        while (cacheSize--) {
            fin.read(reinterpret_cast<char*>(&index), sizeof(index));
            fin.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
            GuessesVec vec(numEntries);
            fin.read(reinterpret_cast<char*>(vec.data()), sizeof(IndexType) * numEntries);
            cache[index] = std::move(vec);
            cacheWsGuesses[index] = WordSetHelpers::buildGuessesWordSet(cache[index]);
        }
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

    static void checkCompression() {
        std::unordered_set<WordSetGuesses> sets = {};
        long long totalCt = 0, setCt = 0;
        for (auto &entry: cacheWsGuesses) {
            sets.insert(entry.second);
            totalCt += entry.second.count();
        }
        for (auto &entry: sets) setCt += entry.count();
        DEBUG("Compressable? " << getPerc(setCt, totalCt));
        exit(1);

    }
};
