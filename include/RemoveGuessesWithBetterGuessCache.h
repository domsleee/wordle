#pragma once
#include "RemoveGuessesWithNoLetterInAnswers.h"
#include "WordSetHelpers.h"
#include "PerfStats.h"
#include "SimpleProgress.h"

#include "Util.h"
struct RemoveGuessesWithBetterGuessCache
{
    static inline std::unordered_map<int, GuessesVec> cache = {};
    static inline std::unordered_map<int, WordSetGuesses> cacheWsGuesses = {};

    static void init()
    {
        START_TIMER(removeguessesbetterguesscache);
        auto expected = 1 << (RemoveGuessesWithNoLetterInAnswers::specialLetters.size());
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
                DEBUG("OH NO " << guesses.size() << " VS " << guessesFast.size());
            }

            totalSize += guessesFast.size();
            cache[nonLetterMask] = guessesFast;
            cacheWsGuesses[nonLetterMask] = WordSetHelpers::buildGuessesWordSet(guessesFast);
            if (cacheWsGuesses.size()%100 == 0) bar.incrementAndUpdateStatus("", 100);
        }
        bar.dispose();
        DEBUG("cache size: " << cache.size() << " totalSize: " << totalSize);
        stats.printEntry("yeet", 4444);
        stats.printEntry("yeet", 4666);
        stats.printEntry("10", 10);
        stats.printEntry("11", 11);
        stats.printEntry("12", 12);
        stats.printEntry("13", 13);

        END_TIMER(removeguessesbetterguesscache);
    }

    static const GuessesVec &getCachedGuessesVec(const AnswersVec &answers)
    {
        return cache[getNonLetterMask(answers)];
    }

    static const WordSetGuesses &getCachedGuessesWordSet(const AnswersVec &answers)
    {
        return cacheWsGuesses[getNonLetterMask(answers)];
    }

    static GuessesVec buildGuessesVecWithoutRemovingAnyAnswer(const GuessesVec &guesses, const AnswersVec &answers)
    {
        GuessesVec vec = guesses;
        auto wsAnswers = WordSetHelpers::buildAnswersWordSet(answers);
        auto &wsGoodGuesses = getCachedGuessesWordSet(answers);
        std::erase_if(vec, [&](const auto guessIndex) -> bool {
            auto isAnAnswer = guessIndex < GlobalState.allAnswers.size() && wsAnswers[guessIndex];
            return !wsGoodGuesses[guessIndex] && !isAnAnswer;
        });
        return vec;
    }

    static int getNonLetterMask(const AnswersVec &answers)
    {
        int answerLetterMask = 0;
        for (auto answerIndex : answers)
        {
            answerLetterMask |= RemoveGuessesWithNoLetterInAnswers::letterCountLookup[answerIndex];
        }
        int nonLetterMask = ~answerLetterMask;
        return nonLetterMask & RemoveGuessesWithNoLetterInAnswers::specialMask;
    }
};
