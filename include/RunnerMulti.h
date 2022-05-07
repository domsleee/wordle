#pragma once
#include <algorithm>
#include <atomic>
#include <execution>

#include "AnswersAndGuessesSolver.h"
#include "Util.h"
#include "RunnerUtil.h"
#include "GlobalArgs.h"
#include "JsonConverter.h"
#include "Verifier.h"
#include "SimpleProgress.h"
#include <type_traits>

struct RunnerMultiResultPair {
    TriesRemainingType numTries;
    IndexType wordIndex;
};

struct RunnerMultiResult {
    std::vector<RunnerMultiResultPair> pairs = {};
    std::shared_ptr<SolutionModel> solutionModel = std::make_shared<SolutionModel>();
};

template <bool parallel>
struct RunnerMulti {
    using P = std::pair<int,int>;
    using ExecutionPolicy = std::conditional<parallel, decltype(std::execution::par_unseq), decltype(std::execution::seq)>::type;
    static constexpr ExecutionPolicy executionPolicy{};

    template <bool isEasyMode, bool isGetLowestAverage>
    int run(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
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
    int runPartitonedByFirstGuess(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        int completed = 0, minWrong = (int)GlobalState.allAnswers.size();
        double minAvg = 500;

        auto guessIndexesToCheck = getIndexesToCheck(nothingSolver);

        auto batchesOfFirstWords = getBatches(guessIndexesToCheck, 8);
        DEBUG("#batches: " << batchesOfFirstWords.size());

        std::mutex lock;

        auto bar = SimpleProgress("BY_FIRST_GUESS", batchesOfFirstWords.size());
        std::ofstream fout("./models/out.res");
        fout << "maxIncorrect: " << GlobalArgs.maxIncorrect << "\n";
        fout << "maxTotalGuesses: " << GlobalArgs.maxTotalGuesses << "\n";
        fout << "word,incorrect,numTries\n";

        std::vector<std::vector<P>> transformResults(batchesOfFirstWords.size());
        std::transform(
            executionPolicy,
            batchesOfFirstWords.begin(),
            batchesOfFirstWords.end(),
            transformResults.begin(),
            [
                &bar,
                &completed,
                &lock,
                &minWrong,
                &minAvg,
                &fout,
                &nothingSolver=std::as_const(nothingSolver),
                &guessIndexesToCheck=std::as_const(guessIndexesToCheck)
            ]
                (const std::vector<IndexType> &firstWordBatch) -> std::vector<P>
            {
                //DEBUG("batch size " << firstWordBatch.size());
                
                const auto &answers = GlobalState.allAnswers;
                const auto &guesses = GlobalState.allGuesses;
                
                std::vector<P> results(firstWordBatch.size());
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < firstWordBatch.size(); ++i) {
                    auto firstWordIndex = firstWordBatch[i];
                    const auto &firstWord = guesses[firstWordIndex];
                    solver.startingWord = firstWord;

                    int incorrect = 0, numTries = 0;
                    for (std::size_t answerIndex = 0; answerIndex < answers.size(); ++answerIndex) {
                        if (incorrect > GlobalArgs.maxIncorrect) {
                            continue;
                        }
                        auto p = AnswerGuessesIndexesPair<TypeToUse>(answers.size(), guesses.size());
                        auto solverRes = solver.solveWord(answerIndex, std::make_shared<SolutionModel>(), firstWordIndex, p);
                        incorrect += solverRes.tries == TRIES_FAILED;
                        numTries += solverRes.tries == TRIES_FAILED ? 0 : solverRes.tries;
                    }

                    {
                        std::lock_guard g(lock);
                        if (incorrect < minWrong) {
                            minWrong = incorrect;
                        }
                        double avg = safeDivide(numTries, GlobalState.allAnswers.size() - incorrect);
                        if (avg < minAvg) {
                            minAvg = avg;
                        }
                        completed++;
                        fout << firstWord << "," << incorrect << "," << numTries << '\n';
                    }
                    fout.flush();

                    auto s = FROM_SS(
                        ", " << firstWord << ", " << getFrac(completed, guessIndexesToCheck.size())
                        << ", minWrong: " << minWrong
                        << ", minAvg: " << minAvg);
                    if (i == firstWordBatch.size() - 1) {
                        bar.incrementAndUpdateStatus(s);
                    } else {
                        bar.updateStatus(s);
                    }
                    results[i] = {answers.size() - incorrect, firstWordIndex};
                }
                
                return results;
            }
        );
        fout.close();

        RunnerUtil::printInfo(nothingSolver, {});
        //DEBUG("guess word ct " << AttemptStateFast::guessWordCt);
        return 0;
    }

    template<bool isEasyMode, bool isGetLowestAverage>
    std::vector<IndexType> getIndexesToCheck(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        auto &vec = GlobalState.allGuesses;
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
        if (GlobalArgs.topLevelGuesses < static_cast<int>(indexesToCheck.size())) {
            std::array<int, NUM_PATTERNS> equiv;
            const auto &answerVec = getVector<UnorderedVec>(GlobalState.allAnswers.size(), 0);
            AnswerGuessesIndexesPair<UnorderedVec> p(answerVec, UnorderedVec(indexesToCheck));
            nothingSolver.calcSortVectorAndGetMinNumWrongFor2(p, equiv);
            p.guesses.sortBySortVec();

            auto sizeOfRes = std::min(static_cast<int>(p.guesses.size()), GlobalArgs.topLevelGuesses);
            indexesToCheck = {p.guesses.begin(), p.guesses.begin() + sizeOfRes};
        }
        return indexesToCheck;
    }

    // since the first guess is fixed, partitioning by pattern reduces the amount of work
    template <bool isEasyMode, bool isGetLowestAverage>
    int runPartitionedByActualAnswer(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        std::atomic<int> completed = 0, incorrect = 0, totalSum = 0, batches = 0;

        auto answerIndexesToCheck = getVector(GlobalState.allAnswers.size(), 0);
        auto batchesOfAnswerIndexes = getBatchesByPattern(nothingSolver, answerIndexesToCheck);
        const auto numBatches = batchesOfAnswerIndexes.size();

        auto bar = SimpleProgress(FROM_SS("BY_ANSWER " << nothingSolver.startingWord), numBatches);

        std::vector<RunnerMultiResult> transformResults(batchesOfAnswerIndexes.size());
        std::transform(
            executionPolicy,
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
                (const std::vector<IndexType> &answerIndexBatch) -> RunnerMultiResult
            {
                RunnerMultiResult result = {};
                result.pairs.resize(answerIndexBatch.size(), {0, 0});
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < answerIndexBatch.size(); ++i) {
                    auto answerIndex = answerIndexBatch[i];
                    const auto &actualAnswer = GlobalState.allAnswers[answerIndex];
                    //DEBUG("CHECKING ANSWER: " << actualAnswer << " first in batch: " << GlobalState.allAnswers[answerIndexBatch[0]]);
                    
                    auto r = solver.solveWord(actualAnswer, result.solutionModel);
                    //if (i == 0) DEBUG("FIRST IN BATCH: " << actualAnswer << ", numWrong: " << r.firstGuessResult.numWrong);
                    completed++;
                    incorrect += r.tries == TRIES_FAILED;
                    totalSum += r.tries != TRIES_FAILED ? r.tries : 0;
                    std::string s = FROM_SS(
                            ", " << actualAnswer << ", " << getFrac(completed.load(), answerIndexesToCheck.size())
                            << " (avg: " << getDivided(totalSum.load(), completed.load() - incorrect.load())
                            << ", incorrect: " << incorrect.load() << ")");
                    
                    if (i == answerIndexBatch.size() - 1) {
                        bar.incrementAndUpdateStatus(s);
                    } else {
                        bar.updateStatus(s);
                    }

                    result.pairs[i] = {r.tries, answerIndex};
                }

                return result;
            }
        );

        auto solutionModel = SolutionModel();
        solutionModel.guess = nothingSolver.startingWord;
        std::vector<int64_t> answerIndexToResult(GlobalState.allAnswers.size());
        for (const auto &r: transformResults) {
            for (const auto &rr: r.pairs) {
                answerIndexToResult[rr.wordIndex] = rr.numTries;
            }
            solutionModel.mergeFromOtherModel(r.solutionModel);

        }
        RunnerUtil::printInfo(nothingSolver, answerIndexToResult);

        JsonConverter::toFile(solutionModel, "./models/model.json");
        if (incorrect == 0) {
            Verifier::verifyModel(solutionModel, nothingSolver);
        } else {
            DEBUG("skip verification... " << incorrect);
        }
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

        auto p = AnswerGuessesIndexesPair<UnorderedVector<IndexType>>(GlobalState.allAnswers.size(), GlobalState.allGuesses.size());
        auto guessIndex = GlobalState.getIndexForWord(nothingSolver.startingWord);

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
