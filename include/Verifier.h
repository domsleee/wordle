#pragma once
#include <string>
#include "JsonConverter.h"
#include "SolutionModel.h"
#include "AnswersAndGuessesSolver.h"

struct Verifier {
    template <bool isEasyMode, bool isGetLowestAverage>
    static std::vector<int64_t> verifyModel(const SolutionModel &model, const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        auto answerIndexes = getVector(nothingSolver.allAnswers.size(), 0);
        std::vector<int64_t> results(answerIndexes.size());
        auto solver = nothingSolver;
        for (auto answerIndex: answerIndexes) {
            auto localModel = model;
            auto answer = solver.reverseIndexLookup[answerIndex];
            auto getter = PatternGetter(answer);
            std::string path = "$";

            auto getterCached = PatternGetterCached(answerIndex);
            auto state = AttemptStateToUse(getterCached);
            auto p = AnswerGuessesIndexesPair<TypeToUse>(nothingSolver.allAnswers.size(), nothingSolver.allGuesses.size());

            for (auto i = 1; i <= solver.maxTries; ++i) {
                auto guessIndex = solver.getIndexForWord(localModel.guess);

                if (guessIndex == answerIndex) {
                    results[answerIndex] = i;
                    if (i > 4) {
                        //DEBUG(answer);
                    }
                    break;
                }
                checkMatches(localModel, solver.reverseIndexLookup, p);
                auto patternStr = getter.getPatternFromWord(localModel.guess);
                //DEBUG(answer << " guess with " << localModel.guess << " " << patternStr);
                solver.makeGuess(p, state, guessIndex);  

                if (!localModel.next.contains(patternStr)) {
                    crashDueToMissingPattern(localModel, patternStr, answer, path);
                    break;
                } else {
                    localModel = *localModel.next[patternStr];
                    path += ".next[\"" + patternStr + "\"]";
                }
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
        //exit(1);
    }

    static void checkMatches(const SolutionModel &model, const std::vector<std::string> &reverseIndexLookup, const AnswerGuessesIndexesPair<TypeToUse> &p) {
        auto subtreeGuesses = SolutionModel::getAllGuessesFromNext(model);
        std::set<std::string> answersFromPair = {};
        for (std::size_t i = 0; i < p.answers.size(); ++i) {
            answersFromPair.insert(reverseIndexLookup[p.answers[i]]);
        }

        if (false && answersFromPair != subtreeGuesses) {
            DEBUG("NOT MATCH!!!" << answersFromPair.size() << " VS " << subtreeGuesses.size());
            printIterable(setDiff(answersFromPair, subtreeGuesses));
            DEBUG("otherdiff");
            printIterable(setDiff(subtreeGuesses, answersFromPair));
            exit(1);
        }
    }

    static std::set<std::string> setDiff(const std::set<std::string> &a, const std::set<std::string> &b) {
        std::set<std::string> res = {};
        for (const auto &aa: a) if (!b.contains(aa)) res.insert(aa);
        return res;
    }
};
