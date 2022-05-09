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
    RemDepthType numTries;
    IndexType wordIndex;
};

struct RunnerMultiResult {
    std::vector<RunnerMultiResultPair> pairs = {};
    std::shared_ptr<SolutionModel> solutionModel = std::make_shared<SolutionModel>();
};

template <bool parallel>
struct RunnerMulti {
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
        int completed = 0, minWrong = INF_INT;
        double minAvg = INF_DOUBLE;

        auto guessIndexesToCheck = getGuessIndexesToCheck(nothingSolver);

        auto batchesOfFirstWords = getBatches(guessIndexesToCheck, 8);
        DEBUG("#batches: " << batchesOfFirstWords.size());

        std::mutex lock;

        auto bar = SimpleProgress("BY_FIRST_GUESS", batchesOfFirstWords.size());
        std::ofstream fout("./models/out.res");
        fout << "maxWrong: " << GlobalArgs.maxWrong << "\n";
        fout << "maxTotalGuesses: " << GlobalArgs.maxTotalGuesses << "\n";
        fout << "word,numWrong,numTries\n";

        std::vector<int> transformResults(batchesOfFirstWords.size());
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
                (const std::vector<IndexType> &firstWordBatch) -> int
            {
                //DEBUG("batch size " << firstWordBatch.size());
                
                const auto &allAnswers = GlobalState.allAnswers;
                const auto &allGuesses = GlobalState.allGuesses;
                
                auto solver = nothingSolver;

                for (std::size_t i = 0; i < firstWordBatch.size(); ++i) {
                    auto firstWordIndex = firstWordBatch[i];
                    const auto &firstWord = allGuesses[firstWordIndex];
                    solver.startingWord = firstWord;

                    int numWrong = 0, numTries = 0;
                    for (std::size_t answerIndex = 0; answerIndex < allAnswers.size(); ++answerIndex) {
                        if (numWrong > GlobalArgs.maxWrong) {
                            numWrong = INF_INT; // invalid
                            continue;
                        }
                        auto answers = getVector<AnswersVec>(allAnswers.size());
                        auto guesses = getVector<GuessesVec>(allGuesses.size());
                        auto solverRes = solver.solveWord(answerIndex, std::make_shared<SolutionModel>(), firstWordIndex, answers, guesses);
                        numWrong += solverRes.tries == TRIES_FAILED;
                        numTries += solverRes.tries == TRIES_FAILED ? 0 : solverRes.tries;
                    }

                    {
                        std::lock_guard g(lock);
                        if (numWrong < minWrong) {
                            minWrong = numWrong;
                        }
                        double avg = numWrong == INF_INT
                            ? INF_DOUBLE
                            : safeDivide(numTries, GlobalState.allAnswers.size() - numWrong);
                        if (avg < minAvg) {
                            minAvg = avg;
                        }
                        completed++;
                        fout << firstWord << "," << numWrong << "," << numTries << '\n';
                    }
                    fout.flush();

                    auto s = FROM_SS(
                        ", " << firstWord << ", " << getFrac(completed, guessIndexesToCheck.size())
                        << ", minWrong: " << (minWrong == INF_INT ? "inf" : std::to_string(minWrong))
                        << ", minAvg: " << (minAvg == INF_DOUBLE ? "inf" : std::to_string(minAvg)));
                    if (i == firstWordBatch.size() - 1) {
                        bar.incrementAndUpdateStatus(s);
                    } else {
                        bar.updateStatus(s);
                    }
                }
                
                return 0;
            }
        );
        fout.close();

        RunnerUtil::printInfo(nothingSolver, {});
        //DEBUG("guess word ct " << AttemptStateFast::guessWordCt);
        return 0;
    }

    template<bool isEasyMode, bool isGetLowestAverage>
    std::vector<IndexType> getGuessIndexesToCheck(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        auto guessIndexes = getVector<GuessesVec>(GlobalState.allGuesses.size(), 0);
        auto prevSize = guessIndexes.size();
        if (GlobalArgs.guessesToSkip != "") {
            auto guessWordsToSkip = readFromFile(GlobalArgs.guessesToSkip);
            std::erase_if(guessIndexes, [&](int guessIndex) {
                return getIndex(guessWordsToSkip, GlobalState.allGuesses[guessIndex]) != -1;
            });
            DEBUG("erased " << (prevSize - guessIndexes.size()) << " words from " << GlobalArgs.guessesToSkip);
        }
        if (GlobalArgs.guessesToCheck != "") {
            auto guessWordsToCheck = readFromFile(GlobalArgs.guessesToCheck);
            guessIndexes.clear();
            for (const auto &w: guessWordsToCheck) guessIndexes.push_back(getIndex(GlobalState.allGuesses, w));
        }
        std::array<int, NUM_PATTERNS> equiv;
        const auto &answerVec = getVector<AnswersVec>(GlobalState.allAnswers.size());
        std::array<int64_t, MAX_NUM_GUESSES> sortVec = {};
        nothingSolver.calcSortVectorAndGetMinNumWrongFor2(answerVec, guessIndexes, equiv, sortVec);
        std::sort(guessIndexes.begin(), guessIndexes.end(), [&](IndexType a, IndexType b) { return sortVec[a] < sortVec[b]; });

        if (GlobalArgs.topLevelGuesses < static_cast<int>(guessIndexes.size())) {
            auto sizeOfRes = GlobalArgs.topLevelGuesses;
            guessIndexes.resize(sizeOfRes);
        }
        return guessIndexes;
    }

    // since the first guess is fixed, partitioning by pattern reduces the amount of work
    template <bool isEasyMode, bool isGetLowestAverage>
    int runPartitionedByActualAnswer(const AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage> &nothingSolver) {
        std::atomic<int> completed = 0, numWrong = 0, totalSum = 0, batches = 0;

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
                &numWrong,
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
                    numWrong += r.tries == TRIES_FAILED;
                    totalSum += r.tries != TRIES_FAILED ? r.tries : 0;
                    std::string s = FROM_SS(
                            ", " << actualAnswer << ", " << getFrac(completed.load(), answerIndexesToCheck.size())
                            << " (avg: " << getDivided(totalSum.load(), completed.load() - numWrong.load())
                            << ", numWrong: " << numWrong.load() << ")");
                    
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
        Verifier::verifyModel(solutionModel, nothingSolver, numWrong);

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

        auto guessIndex = GlobalState.getIndexForWord(nothingSolver.startingWord);

        for (auto answerIndex: indexes) {
            auto getter = PatternGetterCached(answerIndex);
            auto patternInt = getter.getPatternIntCached(guessIndex);
            indexKeys.emplace(answerIndex, patternInt);
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
