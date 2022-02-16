#pragma once
#include <algorithm>
#include <atomic>
#include <execution>
#include "Util.h"

struct MultiRunner {
    static void run(const std::vector<std::string> &answers, const std::vector<std::string> &guesses) {
        START_TIMER(total);
        using P = std::pair<int,int>;
        std::atomic<int> completed = 0, solved = 0;

        if (!completed.is_lock_free()) {
            DEBUG("not lock free!");
            exit(1);
        }

        auto nothingSolver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
        AttemptStateFast::buildForReverseIndexLookup(nothingSolver.reverseIndexLookup);

        auto batchesOfFirstWords = getBatches(guesses.size(), 1);

        DEBUG("#batches: " << batchesOfFirstWords.size());
        std::vector<IndexType> allAnswerIndexes = getVector(answers.size(), 0),
            allGuessIndexes = getVector(guesses.size(), answers.size());

        std::vector<std::vector<P>> transformResults(batchesOfFirstWords.size());
        std::transform(
            std::execution::par_unseq,
            batchesOfFirstWords.begin(),
            batchesOfFirstWords.end(),
            transformResults.begin(),
            [
                &completed,
                &solved,
                &nothingSolver=std::as_const(nothingSolver),
                &guesses=std::as_const(guesses),
                &answers=std::as_const(answers),
                &allAnswerIndexes=std::as_const(allAnswerIndexes),
                &allGuessIndexes=std::as_const(allGuessIndexes)
            ]
                (std::vector<int> &firstWordBatch) -> std::vector<P>
            {
                //DEBUG("batch size " << firstWordBatch.size());
                //auto solver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
                
                std::vector<P> results(firstWordBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < firstWordBatch.size(); ++i) {
                    if (solved.load() > 0) return {};
                    auto firstWordIndex = firstWordBatch[i];
                    const auto &firstWord = guesses[firstWordIndex];

                    //DEBUG("solver cache size " << solver.getBestWordCache.size());

                    std::size_t correct = 0;
                    solver.startingWord = firstWord;
                    for (std::size_t j = 0; j < answers.size(); ++j) {
                        if (j - correct > MAX_INCORRECT) {
                            continue;
                        }
                        const auto &wordToSolve = answers[j];
                        if (solved.load() > 0) return {};
                        auto p = AnswersGuessesIndexesPair(answers.size(), guesses.size());
                        auto r = solver.solveWord(wordToSolve, firstWordIndex, p);
                        correct += r != -1;
                        if (EARLY_EXIT && r == -1) break;
                    }
                    completed++;
                    DEBUG(firstWord << ", completed: " << getPerc(completed.load(), guesses.size()));
                    results[i] = {correct, firstWordIndex};
                    if (correct == answers.size()) {
                        solved++;
                        break;
                    }
                }
                
                return results;
            }
        );

        std::vector<P> results(guesses.size());
        int ind = 0;
        for (const auto &batchResult: transformResults) {
            for (const auto &p: batchResult) results[ind++] = p;
        }

        std::sort(results.begin(), results.end(), std::greater<P>());
        DEBUG("solved " << transformResults.size() << " batches of ~" << transformResults[0].size());
        DEBUG("best result: " << guesses[results[0].second] << " solved " << getPerc(results[0].first, answers.size()));
        std::vector<long long> v(results.size());
        for (std::size_t i = 0; i < results.size(); ++i) {
            v[i] = (int)i < results[0].first;
        }
        
        RunnerUtil::printInfo(nothingSolver, v, guesses, answers);
        END_TIMER(total);
        DEBUG("guess word ct " << AttemptStateFast::guessWordCt);
        exit(1);
    }

    static std::vector<std::vector<int>> getBatches(int n, int batchSize) {
        std::vector<std::vector<int>> res = {};
        for (int i = 0; i < n;) {
            std::vector<int> inner = {};
            int j = i;
            for (;j < std::min(n, i + batchSize); ++j) {
                inner.push_back(j);
            }
            res.push_back(inner);
            i = j;
        }
        return res;
    }
};
