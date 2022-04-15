#pragma once
#include <algorithm>
#include <atomic>
#include <execution>

#include "AnswersAndGuessesSolver.h"
#include "Util.h"
#include "RunnerUtil.h"
#include "GlobalArgs.h"

struct RunnerMulti {
    using P = std::pair<int,int>;

    template <bool isEasyMode, bool isGetLowestAverage>
    static int run(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        START_TIMER(total);
        if (!std::atomic<int>().is_lock_free()) {
            DEBUG("not lock free!");
            exit(1);
        }

        auto r = nothingSolver.startingWord != ""
            ? runPartitionedByActualAnswer(nothingSolver)
            : runPartitonedByFirstGuess(nothingSolver);

        END_TIMER(total);
        return r;
    }

    template <bool isEasyMode, bool isGetLowestAverage>
    static int runPartitonedByFirstGuess(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        std::atomic<int> completed = 0, solved = 0, bestIncorrect = (int)nothingSolver.allAnswers.size();

        auto guessIndexesToCheck = getIndexesToCheck(nothingSolver.allGuesses);

        auto batchesOfFirstWords = getBatches(guessIndexesToCheck, 8);
        DEBUG("#batches: " << batchesOfFirstWords.size());
        //batchesOfFirstWords = {{1285}};

        std::vector<std::vector<P>> transformResults(batchesOfFirstWords.size());
        std::transform(
            std::execution::par_unseq,
            batchesOfFirstWords.begin(),
            batchesOfFirstWords.end(),
            transformResults.begin(),
            [
                &completed,
                &solved,
                &bestIncorrect,
                &nothingSolver=std::as_const(nothingSolver),
                &guessIndexesToCheck=std::as_const(guessIndexesToCheck)
            ]
                (const std::vector<IndexType> &firstWordBatch) -> std::vector<P>
            {
                //DEBUG("batch size " << firstWordBatch.size());
                //auto solver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
                
                const auto &answers = nothingSolver.allAnswers;
                const auto &guesses = nothingSolver.allGuesses;
                
                std::vector<P> results(firstWordBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < firstWordBatch.size(); ++i) {
                    if (solved.load() > 0) return {};
                    auto firstWordIndex = firstWordBatch[i];
                    const auto &firstWord = guesses[firstWordIndex];
                    DEBUG("CHECKING FIRST WORD: " << firstWord);
                    solver.startingWord = firstWord;

                    //DEBUG("solver cache size " << solver.getBestWordCache.size());

                    int incorrect = 0;
                    for (std::size_t answerIndex = 0; answerIndex < answers.size(); ++answerIndex) {
                        if (incorrect > solver.maxIncorrect) {
                            continue;
                        }
                        if (solved.load() > 0) return {};
                        auto p = AnswersGuessesIndexesPair<TypeToUse>(answers.size(), guesses.size());
                        auto r = solver.solveWord(answerIndex, firstWordIndex, p);
                        incorrect += r == -1;
                    }
                    auto currBestIncorrect = bestIncorrect.load();
                    if (incorrect < currBestIncorrect) {
                        currBestIncorrect = incorrect;
                        bestIncorrect = incorrect;
                    }
                    completed++;
                    DEBUG(firstWord << ", completed: " << getPerc(completed.load(), guessIndexesToCheck.size()) << " (" << incorrect << "), best incorrect: " << currBestIncorrect);
                    results[i] = {answers.size() - incorrect, firstWordIndex};
                    if (incorrect == 0) {
                        solved++;
                        break;
                    }
                }
                
                return results;
            }
        );

        std::vector<P> results(nothingSolver.allGuesses.size());
        int ind = 0;
        for (const auto &batchResult: transformResults) {
            for (const auto &p: batchResult) results[ind++] = p;
        }

        std::sort(results.begin(), results.end(), std::greater<P>());
        DEBUG("solved " << transformResults.size() << " batches of ~" << transformResults[0].size());
        DEBUG("best result: " << nothingSolver.allGuesses[results[0].second] << " solved " << getPerc(results[0].first, nothingSolver.allAnswers.size()));
        std::vector<long long> v(results.size());
        for (std::size_t i = 0; i < results.size(); ++i) {
            v[i] = (int)i < results[0].first;
        }
        
        RunnerUtil::printInfo(nothingSolver, v);
        DEBUG("guess word ct " << AttemptStateFast::guessWordCt);
        return 0;
    }

    static std::vector<IndexType> getIndexesToCheck(const std::vector<std::string> &vec) {
        auto indexesToCheck = getVector(vec.size(), 0);
        auto prevSize = indexesToCheck.size();
        if (GlobalArgs.guessesToSkip != "") {
            auto guessWordsToSkip = readFromFile(GlobalArgs.guessesToSkip);
            std::erase_if(indexesToCheck, [&](int guessIndex) {
                return getIndex(guessWordsToSkip, vec[guessIndex]) != -1;
            });
            DEBUG("erased " << (prevSize - indexesToCheck.size()) << " words from " << GlobalArgs.guessesToSkip);
        }
        if (GlobalArgs.guessesToCheck != "") {
            auto guessWordsToCheck = readFromFile(GlobalArgs.guessesToCheck);
            indexesToCheck.clear();
            for (const auto &w: guessWordsToCheck) indexesToCheck.push_back(getIndex(vec, w));
        }
        return indexesToCheck;
    }

    template <bool isEasyMode, bool isGetLowestAverage>
    static int runPartitionedByActualAnswer(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        std::atomic<int> completed = 0, incorrect = 0, totalSum = 0;

        auto answerIndexesToCheck = getIndexesToCheck(nothingSolver.allAnswers);

        //auto batchesOfAnswerIndexes = getBatchesSmart(nothingSolver, answerIndexesToCheck, 8);
        auto batchesOfAnswerIndexes = getBatches(answerIndexesToCheck, 8);

        DEBUG("#batches: " << batchesOfAnswerIndexes.size());

        std::vector<std::vector<P>> transformResults(batchesOfAnswerIndexes.size());
        std::transform(
            std::execution::par_unseq,
            batchesOfAnswerIndexes.begin(),
            batchesOfAnswerIndexes.end(),
            transformResults.begin(),
            [
                &completed,
                &incorrect,
                &totalSum,
                &nothingSolver=std::as_const(nothingSolver),
                &answerIndexesToCheck=std::as_const(answerIndexesToCheck)
            ]
                (const std::vector<IndexType> &answerIndexBatch) -> std::vector<P>
            {   
                std::vector<P> results(answerIndexBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < answerIndexBatch.size(); ++i) {
                    auto answerIndex = answerIndexBatch[i];
                    const auto &actualAnswer = nothingSolver.allAnswers[answerIndex];
                    DEBUG("CHECKING ANSWER: " << actualAnswer);
                    
                    auto r = solver.solveWord(actualAnswer);
                    completed++;
                    incorrect += r == -1;
                    totalSum += r != -1 ? r : 0;
                    DEBUG(actualAnswer << ", completed: " << getPerc(completed.load(), answerIndexesToCheck.size())
                            << " (avg: " << getDivided(totalSum.load(), completed.load() - incorrect.load())
                            << ", incorrect: " << incorrect << ")");
                    results[i] = {r, answerIndex};
                }
                
                return results;
            }
        );

        std::vector<long long> answerIndexToResult(nothingSolver.allAnswers.size());
        for (const auto &r: transformResults) {
            for (const auto &rr: r) {
                answerIndexToResult[rr.second] = rr.first;
            }
        }
        RunnerUtil::printInfo(nothingSolver, answerIndexToResult);
        return 0;
    }

    static std::vector<std::vector<IndexType>> getBatches(const std::vector<IndexType> &indexes, std::size_t batchSize) {
        std::vector<std::vector<IndexType>> res = {};
        for (std::size_t i = 0; i < indexes.size();) {
            std::vector<IndexType> inner = {};
            std::size_t j = i;
            for (;j < std::min(indexes.size(), i + batchSize); ++j) {
                inner.push_back(indexes[j]);
            }
            res.push_back(inner);
            i = j;
        }
        return res;
    }

    template <bool isEasyMode, bool isGetLowestAverage>
    static std::vector<std::vector<IndexType>> getBatchesSmart(
        const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver,
        const std::vector<IndexType> &indexes,
        std::size_t batchSize)
    {
        std::unordered_map<AnswersAndGuessesKey, std::vector<IndexType>> keyToIndexes = {};
        auto p = AnswersGuessesIndexesPair<UnorderedVector<IndexType>>(nothingSolver.allAnswers.size(), nothingSolver.allGuesses.size());
        auto answerIndex = nothingSolver.getIndexFromStartingWord();
        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);

        DEBUG("start: " << p.answers.size() << ", " << p.guesses.size());

        for (auto guessIndex: indexes) {
            nothingSolver.makeGuess(p, state, guessIndex, nothingSolver.reverseIndexLookup);
                    DEBUG("eval: " << p.answers.size() << ", " << p.guesses.size());

            auto key = AnswersAndGuessesSolver<false, false>::getCacheKey(p.answers, p.guesses, 0);
            if (!keyToIndexes.contains(key)) keyToIndexes[key] = {};
            keyToIndexes[key].push_back(guessIndex);
            p.answers.restoreAllValues();
            p.guesses.restoreAllValues();

        }

        DEBUG("num groups: " << indexes.size());
        exit(1);
        return {};
    }
};
