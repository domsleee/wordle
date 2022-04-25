#pragma once
#include "GuessesRemainingAfterGuessCache.h"

#define BETTER_GUESS_OPT_DEBUG(x)


struct RemoveGuessesBetterGuess {
    static std::size_t removeGuessesWhichHaveBetterGuess(AnswerGuessesIndexesPair<UnorderedVec> &p, bool force = false) {
        return 0;
        const auto answersSize = p.answers.size();
        const auto &pAnswersWs = WordSetHelpers::buildAnswersWordSet(p.answers);

        std::vector<IndexType> indexOfPGuessesToConsider = {};
        indexOfPGuessesToConsider.reserve(p.guesses.size());

        for (std::size_t indexOfPGuesses = p.guesses.size()-1;;--indexOfPGuesses) {
            auto guessIndex = p.guesses[indexOfPGuesses];
            if (guessIndex >= pAnswersWs.size() || !pAnswersWs[guessIndex]) {
                indexOfPGuessesToConsider.push_back(indexOfPGuesses);
            }
            if (indexOfPGuesses == 0) break;
        }

#define GEN_CONDITION(x) (answersSize < x) removed = getRemovedUsingBitset<x>(p, indexOfPGuessesToConsider);
        std::size_t removed = 0;
        if GEN_CONDITION(64)
        else if GEN_CONDITION(128)
        else if GEN_CONDITION(192)
        else if GEN_CONDITION(256)
        else if GEN_CONDITION(320)
        else if GEN_CONDITION(384)
        else if GEN_CONDITION(576)
        else if GEN_CONDITION(2315);

        if (removed > 0) {
            BETTER_GUESS_OPT_DEBUG("REMOVed by equal: " << getPerc(removed, guessesSize)
                << " #answers: " << p.answers.size()
                << " #wsAnswerMap: " << wsAnswerMap.size());
        }
        return removed;
    }

    template<int bitsetSize>
    static std::size_t getRemovedUsingBitset(
        AnswerGuessesIndexesPair<UnorderedVec> &p,
        const std::vector<IndexType> &indexOfPGuessesToConsider)
    {
        using MyBitset = std::bitset<bitsetSize>;
        const auto answersSize = p.answers.size();
        //std::unordered_map<int, int> seenWsAnswerIndexes = {};
        std::unordered_map<MyBitset, std::size_t> wsAnswerMap = {};
        std::unordered_map<std::vector<int>, std::size_t, VectorHasher> wsToIndex = {};
        std::vector<std::size_t> equalTo(indexOfPGuessesToConsider.size());
        for (std::size_t i = 0; i < indexOfPGuessesToConsider.size(); ++i) equalTo[i] = i;

        //counting sort??
        std::size_t removed = 0;
        for (std::size_t i = 0; i < indexOfPGuessesToConsider.size(); ++i) {
            auto indexOfPGuesses = indexOfPGuessesToConsider[i];
            auto guessIndex = p.guesses[indexOfPGuesses];
            auto wsAnswerVec = std::vector<int>(answersSize);
            for (std::size_t j = 0; j < answersSize; ++j) {
                auto compressedBitset = MyBitset();
                auto answerIndex = p.answers[j];
                const auto& wsAnswersForGuess = GuessesRemainingAfterGuessCache::getFromCacheWithAnswerIndex(guessIndex, answerIndex);
                for (std::size_t k = 0; k < answersSize; ++k) {
                    if (wsAnswersForGuess[p.answers[k]]) compressedBitset.set(k);
                }

                auto myPair = wsAnswerMap.try_emplace(compressedBitset, wsAnswerMap.size());
                wsAnswerVec[j] = (*myPair.first).second;
            }
            //answerReductions.emplace_back(node);
            auto myPair = wsToIndex.try_emplace(wsAnswerVec, i);
            if (!myPair.second) {
                equalTo[i] = (*myPair.first).second;
            }
            if (equalTo[i] != i) {
                p.guesses.deleteIndex(indexOfPGuesses);
                removed++;
            }
        }
        return removed;
    }

    struct MyNode {
        MyNode(IndexType indexOfPGuesses): indexOfPGuesses(indexOfPGuesses) {}
        MyNode(){}
        IndexType indexOfPGuesses = 0;
        std::vector<int> wsAnswerVec = {};
    };

    struct VectorHasher {
        int operator()(const std::vector<int> &V) const {
            int hash = V.size();
            for(auto &i : V) {
                hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            }
            return hash;
        }
    };
};