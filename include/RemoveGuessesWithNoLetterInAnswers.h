#pragma once
#include "GlobalState.h"

struct RemoveGuessesWithNoLetterInAnswers {
    inline static std::vector<int> letterCountLookup = {};

    /*static std::vector<IndexType> clearGuesses(std::vector<IndexType> guesses, const std::vector<IndexType> &answers) {   
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        std::erase_if(guesses, [&](const int &guessIndex) {
            return (answerLetterMask & letterCountLookup[guessIndex]) == 0;
        });
        return guesses;
    }&*/

    static void clearGuesses(GuessesVec &guesses, const AnswersVec &answers) {
        auto ratio = (double)guesses.size() / answers.size();
        
        if (ratio <= 2.00) {
            //DEBUG("skipping...");
            return;
        }
        static int call = 0;
        //DEBUG("call " << ++call);
        int answerLetterMask = 0;
        for (auto answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        if (__builtin_popcount(answerLetterMask) >= 21) {
            static int hmm = 0;
            //DEBUG("HMM" << (++hmm));
            return;
        }
        std::size_t deleted = 0;
        for (std::size_t i = 0; i < guesses.size() - deleted; ++i) {
            if ((answerLetterMask & letterCountLookup[guesses[i]]) == 0) {
                std::swap(guesses[i], guesses[guesses.size()-1-deleted]);
                deleted++;
                --i;
            }
        }
        guesses.resize(guesses.size() - deleted);
        //DEBUG("sending... " << ratio << " #guesses: " << guesses.size() << ", #answers: " << answers.size() << ", removed: " << deleted);
    }

    // clear guesses with equal or better guess...
    // looking at answerLetterMask

    static std::size_t clearGuesses(UnorderedVector<IndexType> &guesses, const UnorderedVector<IndexType> &answers) {
        std::size_t guessesRemoved = 0;
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        for (std::size_t i = guesses.size()-1; i != MAX_SIZE_VAL; --i) {
            auto guessIndex = guesses[i];
            if ((answerLetterMask & letterCountLookup[guessIndex]) == 0) {
                guesses.deleteIndex(i);
                guessesRemoved++;
            }
        }

        return guessesRemoved;
    }

    static void buildClearGuessesInfo() {
        if (letterCountLookup.size() > 0) return;
        letterCountLookup.resize(GlobalState.reverseIndexLookup.size());
        for (std::size_t i = 0; i < GlobalState.reverseIndexLookup.size(); ++i) {
            letterCountLookup[i] = 0;
            for (auto c: GlobalState.reverseIndexLookup[i]) {
                letterCountLookup[i] |= (1 << (c-'a'));
            }
        }
    }
};
