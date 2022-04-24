#pragma once
#include <string>
#include "JsonConverter.h"
#include "SolutionModel.h"
#include "AnswersAndGuessesSolver.h"

struct Verifier {
    template <bool isEasyMode, bool isGetLowestAverage>
    static std::vector<int64_t> verifyModel(const SolutionModel &model, const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &solver) {
        auto answerIndexes = getVector(solver.allAnswers.size(), 0);
        std::vector<int64_t> results(answerIndexes.size());
        for (auto answerIndex: answerIndexes) {
            auto localModel = model;
            auto answer = solver.reverseIndexLookup[answerIndex];
            auto getter = PatternGetter(answer);
            std::string path = "$";
            for (auto i = 1; i <= solver.maxTries; ++i) {
                auto guessIndex = solver.getIndexForWord(localModel.guess);
                if (guessIndex == answerIndex) {
                    results[answerIndex] = i;
                    break;
                }
                auto patternStr = getter.getPatternFromWord(localModel.guess);

                if (!localModel.next.contains(patternStr)) {
                    crashDueToMissingPattern(localModel, patternStr, answer, path);
                }
                localModel = *localModel.next[patternStr];
                path += ".next[\"" + patternStr + "\"]";
            }
        }

        long above4 = 0;
        for (auto r: results) above4 += r > 4;
        DEBUG("above4: " << above4);

        return results;
    }

    static void crashDueToMissingPattern(
        const SolutionModel &localModel,
        const std::string &patternStr,
        const std::string &answer,
        const std::string &path)
    {
        DEBUG("error: does not have next pattern '" << patternStr << "'");
        DEBUG("curren guess: " << localModel.guess);
        for (auto &key: localModel.next) {
            DEBUG(key.first);
        }
        DEBUG("answer: " << answer);
        DEBUG(path);
        DEBUG("amount: " << localModel.next.size());
        exit(1);
    }
};
