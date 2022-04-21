#pragma once
#include <algorithm>
#include <atomic>
#include <execution>

#include "AnswersAndGuessesSolver.h"
#include "Util.h"
#include "RunnerUtil.h"
#include "GlobalArgs.h"

const int RESULT_INCORRECT = -1;
const int RESULT_SKIPPED = -2;

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
                
                const auto &answers = nothingSolver.allAnswers;
                const auto &guesses = nothingSolver.allGuesses;
                
                std::vector<P> results(firstWordBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < firstWordBatch.size(); ++i) {
                    //if (solved.load() > 0) return {};
                    auto firstWordIndex = firstWordBatch[i];
                    const auto &firstWord = guesses[firstWordIndex];
                    DEBUG("CHECKING FIRST WORD: " << firstWord << " first in batch: " << guesses[firstWordBatch[0]]);
                    solver.startingWord = firstWord;

                    //DEBUG("solver cache size " << solver.getBestWordCache.size());

                    int incorrect = 0;
                    for (std::size_t answerIndex = 0; answerIndex < answers.size(); ++answerIndex) {
                        if (incorrect > solver.maxIncorrect) {
                            continue;
                        }
                        //if (solved.load() > 0) return {};
                        auto p = AnswerGuessesIndexesPair<TypeToUse>(answers.size(), guesses.size());
                        auto solverRes = solver.solveWord(answerIndex, firstWordIndex, p);
                        incorrect += solverRes.tries == -1;
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
        std::vector<int64_t> v(results.size());
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
        std::atomic<int> completed = 0, incorrect = 0, totalSum = 0, batches = 0;

        auto answerIndexesToCheck = getIndexesToCheck(nothingSolver.allAnswers);
        auto batchesOfAnswerIndexes = getBatchesByPattern(nothingSolver, answerIndexesToCheck);
        const auto numBatches = batchesOfAnswerIndexes.size();

        auto bar = SimpleProgress("BY_ANSWER batch", numBatches);

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
                &batches,
                &bar,
                &numBatches=std::as_const(numBatches),
                &nothingSolver=std::as_const(nothingSolver),
                &answerIndexesToCheck=std::as_const(answerIndexesToCheck)
            ]
                (const std::vector<IndexType> &answerIndexBatch) -> std::vector<P>
            {   
                std::vector<P> results(answerIndexBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < answerIndexBatch.size(); ++i) {
                    solver.skipRemoveGuessesWhichHaveBetterGuess = i > 0;
                    auto answerIndex = answerIndexBatch[i];
                    const auto &actualAnswer = nothingSolver.allAnswers[answerIndex];
                    //DEBUG("CHECKING ANSWER: " << actualAnswer << " first in batch: " << nothingSolver.allAnswers[answerIndexBatch[0]]);
                    
                    auto r = solver.solveWord(actualAnswer);
                    //if (i == 0) DEBUG("FIRST IN BATCH: " << actualAnswer << ", probWrong: " << r.firstGuessResult.probWrong);
                    completed++;
                    incorrect += r.tries == -1;
                    totalSum += r.tries != -1 ? r.tries : 0;
                    std::string s = FROM_SS(
                            ", " << actualAnswer << ", " << getFrac(completed.load(), answerIndexesToCheck.size())
                            << " (avg: " << getDivided(totalSum.load(), completed.load() - incorrect)
                            << ", incorrect: " << incorrect.load() << ")");
                    
                    if (i == answerIndexBatch.size() - 1) {
                        bar.incrementAndUpdateStatus(s);
                    } else {
                        bar.updateStatus(s);
                    }

                    results[i] = {r.tries, answerIndex};
                }

                return results;
            }
        );

        std::vector<int64_t> answerIndexToResult(nothingSolver.allAnswers.size());
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
    static std::vector<std::vector<IndexType>> getBatchesByPattern(
        const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver,
        const std::vector<IndexType> &indexes)
    {
        std::map<IndexType, int> indexKeys;
        std::vector<IndexType> sortedIndexes = indexes;

        auto p = AnswerGuessesIndexesPair<UnorderedVector<IndexType>>(nothingSolver.allAnswers.size(), nothingSolver.allGuesses.size());
        auto guessIndex = nothingSolver.getIndexForWord(nothingSolver.startingWord);

        DEBUG("start: " << p.answers.size() << ", " << p.guesses.size());

        for (auto answerIndex: indexes) {
            auto getter = PatternGetterCached(answerIndex);
            auto patternInt = getter.getPatternIntCached(guessIndex);

            indexKeys.emplace(answerIndex, patternInt);
            p.answers.restoreAllValues();
            p.guesses.restoreAllValues();
        }

        std::vector<std::vector<IndexType>> groups = {};
        auto addToGroups = [&](const std::vector<IndexType> &indexesThatAreSorted) {
            for (auto ind: indexesThatAreSorted) {
                auto added = false;
                for (auto &vec: groups) {
                    auto firstRepKey = indexKeys[vec[0]];
                    if (indexKeys[ind] == firstRepKey) {
                        vec.push_back(ind);
                        added = true;
                        break;
                    }
                }
                if (added) continue;
                groups.push_back({ind});
            }
        };
        addToGroups(sortedIndexes);

        DEBUG("num groups: " << groups.size() << " " << getPerc(groups.size(), indexes.size()));
        return groups;
    }
};
