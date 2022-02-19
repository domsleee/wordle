#pragma once
#include <algorithm>
#include <atomic>
#include <execution>
#include "Util.h"

struct MultiRunner {
    static int run(const AnswersAndGuessesSolver<IS_EASY_MODE> &nothingSolver) {
        START_TIMER(total);
        using P = std::pair<int,int>;
        std::atomic<int> completed = 0, solved = 0;

        if (!completed.is_lock_free()) {
            DEBUG("not lock free!");
            exit(1);
        }

        auto batchesOfFirstWords = getBatches(nothingSolver.allGuesses.size(), 8);
        auto it = std::find(nothingSolver.reverseIndexLookup.begin(), nothingSolver.reverseIndexLookup.end(), "least");
        auto indo = std::distance(nothingSolver.reverseIndexLookup.begin(), it);
        DEBUG("INDEX" <<  indo);
        DEBUG("#batches: " << batchesOfFirstWords.size());
        batchesOfFirstWords = {{1285}};

        std::vector<std::vector<P>> transformResults(batchesOfFirstWords.size());
        std::transform(
            std::execution::par_unseq,
            batchesOfFirstWords.begin(),
            batchesOfFirstWords.end(),
            transformResults.begin(),
            [
                &completed,
                &solved,
                &nothingSolver=std::as_const(nothingSolver)
            ]
                (const std::vector<int> &firstWordBatch) -> std::vector<P>
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
                    solver.startingWord = firstWord;

                    //DEBUG("solver cache size " << solver.getBestWordCache.size());

                    std::size_t correct = 0;
                    for (std::size_t j = 0; j < answers.size(); ++j) {
                        if (j - correct > solver.maxIncorrect) {
                            continue;
                        }
                        const auto &wordToSolve = answers[j];
                        if (solved.load() > 0) return {};
                        auto p = AnswersGuessesIndexesPair<std::vector<IndexType>>(answers.size(), guesses.size());
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
        END_TIMER(total);
        DEBUG("guess word ct " << AttemptStateFast::guessWordCt);
        return 0;
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
