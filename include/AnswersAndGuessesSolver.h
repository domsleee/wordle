#pragma once
#include "AttemptState.h"
#include "AttemptStateFaster.h"
#include "AnswersAndGuessesResult.h"
#include "GlobalState.h"

#include "PatternGetter.h"
#include "WordSetUtil.h"
#include "BestWordResult.h"
#include "AnswersAndGuessesKey.h"
#include "AnswerGuessesIndexesPair.h"
#include "Defs.h"
#include "UnorderedVector.h"
#include "RemoveGuessesWithNoLetterInAnswers.h"
#include "RemoveGuessesBetterGuess.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) 

static constexpr int INF_INT = (int)1e8;

template <bool isEasyMode, bool isGetLowestAverage=false>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(TriesRemainingType maxTries)
        : maxTries(maxTries)
        {}

    const TriesRemainingType maxTries;
    bool skipRemoveGuessesWhichHaveBetterGuess = false;
    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, BestWordResult> getGuessCache = {};
    long long cacheMiss = 0, cacheHit = 0;

    static const int isEasyModeVar = isEasyMode;

    AnswersAndGuessesResult solveWord(const std::string &answer, std::shared_ptr<SolutionModel> solutionModel) {
        if (std::find(GlobalState.allAnswers.begin(), GlobalState.allAnswers.end(), answer) == GlobalState.allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        IndexType firstWordIndex;

        auto p = AnswerGuessesIndexesPair<TypeToUse>(GlobalState.allAnswers.size(), GlobalState.allGuesses.size());
        if (startingWord == "") {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getGuessFunctionDecider(p, maxTries, getDefaultMaxNumberIncorrect());
            DEBUG("first word: " << GlobalState.reverseIndexLookup[firstPr.wordIndex] << ", known numWrong: " << firstPr.numWrong);
            firstWordIndex = firstPr.wordIndex;
        } else {
            firstWordIndex = GlobalState.getIndexForWord(startingWord);
        }
        auto answerIndex = GlobalState.getIndexForWord(answer);
        return solveWord(answerIndex, solutionModel, firstWordIndex, p);
    }

    template<class T>
    AnswersAndGuessesResult solveWord(IndexType answerIndex, std::shared_ptr<SolutionModel> solutionModel, IndexType firstWordIndex, AnswerGuessesIndexesPair<T> &p) {
        AnswersAndGuessesResult res = {};
        res.solutionModel = solutionModel;

        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);
        auto currentModel = res.solutionModel;

        //clearGuesses(p.guesses, p.answers);
        //removeGuessesWhichHaveBetterGuess(p, true);
        
        int guessIndex = firstWordIndex;
        for (res.tries = 1; res.tries <= maxTries; ++res.tries) {
            currentModel->guess = GlobalState.reverseIndexLookup[guessIndex];
            if (guessIndex == answerIndex) {
                currentModel->next["+++++"] = std::make_shared<SolutionModel>();
                currentModel->next["+++++"]->guess = currentModel->guess;
                return res;
            }
            if (res.tries == maxTries) break;

            makeGuess(p, state, guessIndex);

            if constexpr (std::is_same<T, UnorderedVec>::value && isEasyMode) {
                RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
                RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p, true);
            }

            auto pr = getGuessFunctionDecider(p, maxTries-res.tries, getDefaultMaxNumberIncorrect());
            if (res.tries == 1) res.firstGuessResult = pr;
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << GlobalState.reverseIndexLookup[pr.wordIndex] << ", numWrong: " << pr.numWrong);

            const auto patternInt = getter.getPatternIntCached(guessIndex);
            const auto patternStr = PatternIntHelpers::patternIntToString(patternInt);
            currentModel = currentModel->getOrCreateNext(patternStr);

            guessIndex = pr.wordIndex;
        }
        res.tries = TRIES_FAILED;
        return res;
    }


    template<typename T>
    void makeGuess(AnswerGuessesIndexesPair<T> &pair, const AttemptStateToUse &state, IndexType guessIndex) const {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            pair.answers = state.guessWord(guessIndex, pair.answers);
            if constexpr (!isEasyMode) pair.guesses = state.guessWord(guessIndex, pair.guesses);
        } else {
            state.guessWordAndRemoveUnmatched(guessIndex, pair.answers);
            if constexpr (!isEasyMode) {
                state.guessWordAndRemoveUnmatched(guessIndex, pair.guesses);
            }
        }
    }

private:
    template <typename T>
    inline BestWordResult getGuessFunctionDecider(
        AnswerGuessesIndexesPair<T> &p,
        const TriesRemainingType triesRemaining,
        const int maxIncorrectForNode) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            return getDefaultBestWordResult();
        } else {
            if constexpr (isGetLowestAverage) {
                return getGuessForLowestAverage(p, triesRemaining, maxIncorrectForNode);
            } else {
                return getGuessForLeastWrong(p, triesRemaining, maxIncorrectForNode);
            }
        }
    }

    template <typename T>
    BestWordResult getGuessForLeastWrong(AnswerGuessesIndexesPair<T> &p, const TriesRemainingType triesRemaining, const int maxIncorrectForNode) {
        assertm(triesRemaining != 0, "no tries remaining");
        if (p.answers.size() == 0) return {0, 0};
        if (triesRemaining >= p.answers.size()) return {0, *std::min_element(p.answers.begin(), p.answers.end())};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                (int)p.answers.size()-1,
                *std::min_element(p.answers.begin(), p.answers.end())
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        auto guessesRemovedByClearGuesses = isEasyMode
            ? RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers)
            : 0;
        if constexpr (std::is_same<T, UnorderedVec>::value && isEasyMode) {
            if (isEasyMode && triesRemaining >= 3) {
                guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
            }
        }
        bool shouldSort = true;
        if (shouldSort) sortGuessesByHeuristic(key, p);
        
        BestWordResult res = getDefaultBestWordResult();

        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(p.guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto &possibleGuess = p.guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(GlobalState.reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, p.guesses.size()));
            int numWrongForThisGuess = 0;
            for (std::size_t i = 0; i < p.answers.size(); ++i) {
                const auto &actualWordIndex = p.answers[i];
                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, actualWordIndex, triesRemaining, maxIncorrectForNode - numWrongForThisGuess);
                const auto expNumWrongForSubtree = pr.numWrong;
                numWrongForThisGuess += expNumWrongForSubtree;
                if (numWrongForThisGuess > res.numWrong) break;
                if (expNumWrongForSubtree > GlobalArgs.maxIncorrect) {
                    numWrongForThisGuess = INF_INT-1;
                    break;
                }
            }

            BestWordResult newRes = {numWrongForThisGuess, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                if (res.numWrong == 0) {
                    if (shouldSort) restoreSort(p);
                    p.guesses.restoreValues(guessesRemovedByClearGuesses);
                    return setCacheVal(key, res);
                }
            }
        }

        if (shouldSort) restoreSort(p);
        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    int getDefaultMaxNumberIncorrect() {
        if constexpr (isGetLowestAverage) {
            return GlobalArgs.maxTotalGuesses;
        } else {
            return GlobalArgs.maxIncorrect;
        }
    }

    BestWordResult getGuessForLowestAverage(AnswerGuessesIndexesPair<UnorderedVec> &p, const TriesRemainingType triesRemaining, const int maxTotalGuessesForThisNode) {
        assertm(triesRemaining != 0, "no tries remaining");

        if (p.answers.size() == 0) return {0, 0};
        if (p.answers.size() == 1) return {1, p.answers[0]};
        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                INF_INT,
                p.answers[0]
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        auto guessesRemovedByClearGuesses = isEasyMode
            ? RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers)
            : 0;

        if (isEasyMode && triesRemaining >= 3) {
            guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
        }

        bool shouldSort = true;
        if (shouldSort) sortGuessesByHeuristic(key, p);
        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(p.guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto &possibleGuess = p.guesses[myInd];            
            int totalNumGuessesForThisGuess = 0;
            for (std::size_t i = 0; i < p.answers.size(); ++i) {
                const auto &actualWordIndex = p.answers[i];
                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, actualWordIndex, triesRemaining, maxTotalGuessesForThisNode - totalNumGuessesForThisGuess);
                totalNumGuessesForThisGuess += pr.numWrong;
                if (totalNumGuessesForThisGuess > res.numWrong) break;
                if (totalNumGuessesForThisGuess > GlobalArgs.maxTotalGuesses) break;
                if (pr.numWrong == INF_INT) {
                    totalNumGuessesForThisGuess = INF_INT-1;
                    break;
                }
            }

            BestWordResult newRes = {totalNumGuessesForThisGuess, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
            }
        }

        if (shouldSort) restoreSort(p);
        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    inline BestWordResult makeGuessAndRestoreAfter(
        AnswerGuessesIndexesPair<UnorderedVec> &p,
        const IndexType possibleGuess,
        const IndexType actualWordIndex,
        const TriesRemainingType triesRemaining,
        const int maxIncorrectForNode)
    {
        const auto getter = PatternGetterCached(actualWordIndex);
        const auto state = AttemptStateToUse(getter);
    
        auto numAnswersRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, p.answers);
        BestWordResult pr;
        if constexpr (isEasyMode) {
            pr = getGuessFunctionDecider(p, triesRemaining-1, maxIncorrectForNode);
        } else {
            auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, p.guesses);
            pr = getGuessFunctionDecider(p, triesRemaining-1, maxIncorrectForNode);
            p.guesses.restoreValues(numGuessesRemoved);
        }
        p.answers.restoreValues(numAnswersRemoved);
        return pr;
    }

    const BestWordResult getFromCache(const AnswersAndGuessesKey<isEasyMode> &key) {
        auto it = getGuessCache.find(key);
        if (it == getGuessCache.end()) {
            cacheMiss++;
            return getDefaultBestWordResult();
        }

        cacheHit++;
        return it->second;
    }

    inline BestWordResult setCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        return getGuessCache[key] = res;
    }

    template<typename T>
    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(const AnswerGuessesIndexesPair<T> &p, const TriesRemainingType triesRemaining) {
        auto key = getCacheKey(p, triesRemaining);
        const auto cacheVal = getFromCache(key);
        return {key, cacheVal};
    }

    template<typename T>
    static AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswerGuessesIndexesPair<T> &p, TriesRemainingType triesRemaining) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, triesRemaining);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, p.guesses, triesRemaining);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF_INT, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }

    void sortGuessesByHeuristic(const AnswersAndGuessesKey<isEasyMode> &cacheKey, AnswerGuessesIndexesPair<UnorderedVec> &p) {
        static thread_local std::array<int, NUM_PATTERNS> patternCt;
        
        for (const auto guessIndex: p.guesses) {
            for (std::size_t i = 0; i < NUM_PATTERNS; ++i) patternCt[i] = -1;
            std::size_t totalAnswersRemaining = 0;
            for (auto answerIndex: p.answers) {
                const auto getter = PatternGetterCached(answerIndex);
                const auto state = AttemptStateToUse(getter);
                const auto patternInt = getter.getPatternIntCached(guessIndex);

                if (patternCt[patternInt] == -1) {
                    patternCt[patternInt] = state.guessWordAndCountRemaining(guessIndex, p.answers);
                }
                totalAnswersRemaining = std::min(static_cast<std::size_t>(INF_INT), totalAnswersRemaining + patternCt[patternInt]);
            }
            p.guesses.registerSortVec(guessIndex, totalAnswersRemaining);
        }

        p.guesses.sortBySortVec();
    }

    void restoreSort(AnswerGuessesIndexesPair<UnorderedVec> &p) {
        p.guesses.restoreSort();
    }
};
