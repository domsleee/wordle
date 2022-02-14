#pragma once
#include <algorithm>
#include <atomic>
#include <execution>
#include "Util.h"

struct MultiRunner {
    static void run(const std::vector<std::string> &answers, const std::vector<std::string> &guesses) {
        using P = std::pair<int,int>;
        std::vector<P> results(guesses.size(), {-1, -1});
        std::vector<int> firstGuessesToTry = getVector<int>(guesses.size(), 0);
        std::atomic<int> completed = 0;

        auto nothingSolver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
        AttemptStateFast::buildForReverseIndexLookup(nothingSolver.reverseIndexLookup);

        std::transform(
            std::execution::par,
            firstGuessesToTry.begin(),
            firstGuessesToTry.end(),
            results.begin(),
            [&completed, &nothingSolver=std::as_const(nothingSolver), &guesses=std::as_const(guesses), &answers=std::as_const(answers)](int firstWordIndex) -> P {
                //auto solver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
                auto solver = nothingSolver;
                auto correct = 0;
                const auto &firstWord = guesses[firstWordIndex];
                solver.startingWord = firstWord;
                for (const auto &wordToSolve: answers) {
                    auto r = solver.solveWord(wordToSolve);
                    correct += r != -1;
                }
                completed++;
                DEBUG(firstWord << ", completed: " << getPerc(completed.load(), guesses.size()));

                return {correct, firstWordIndex};
            }
        );

        std::sort(results.begin(), results.end(), std::greater<P>());
        DEBUG("best result: " << guesses[results[0].second] << " solved " << getPerc(results[0].first, answers.size()));
        std::vector<long long> v(results.size());
        for (std::size_t i = 0; i < results.size(); ++i) {
            v[i] = (int)i < results[0].first;
        }
        
        RunnerUtil::printInfo(nothingSolver, v, guesses, answers);
        exit(1);
    }
};
