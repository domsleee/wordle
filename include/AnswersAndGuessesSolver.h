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
static constexpr double INF_DOUBLE = 1e8;

template <bool isEasyMode, bool isGetLowestAverage=false>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(RemDepthType maxTries)
        : maxTries(maxTries)
        {}

    const RemDepthType maxTries;
    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, BestWordResult> getGuessCache = {};
    long long cacheMiss = 0, cacheHit = 0;
    std::vector<std::array<std::array<int, NUM_PATTERNS>, MAX_NUM_GUESSES>> lbCache3d;

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
            auto firstPr = getGuessFunctionDecider(p, maxTries, getDefaultBeta());
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
        if (lbCache3d.size() == 0 && isGetLowestAverage) lbCache3d.resize(10);
        AnswersAndGuessesResult res = {};
        res.solutionModel = solutionModel;

        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);
        auto currentModel = res.solutionModel;

        //RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
        //RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p, true);
        
        int guessIndex = firstWordIndex;
        for (res.tries = 1; res.tries <= maxTries; ++res.tries) {
            currentModel->guess = GlobalState.reverseIndexLookup[guessIndex];
            if (currentModel->guess == "") {
                DEBUG("FAILED: " << guessIndex); exit(1);
            }
            if (guessIndex == answerIndex) {
                currentModel->next["+++++"] = std::make_shared<SolutionModel>();
                currentModel->next["+++++"]->guess = currentModel->guess;
                return res;
            }
            if (res.tries == maxTries) break;

            makeGuess(p, state, guessIndex);

            if constexpr (std::is_same<T, UnorderedVec>::value && isEasyMode) {
                //RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
                //RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p, true);
            }

            auto pr = getGuessFunctionDecider(p, maxTries-res.tries, getDefaultBeta());
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

    template<typename T>
    static BestWordResult calcSortVectorAndGetMinNumWrongFor2(
        AnswerGuessesIndexesPair<T> &p,
        std::array<int, NUM_PATTERNS> &equiv)
    {
        BestWordResult minNumWrongFor2 = {INF_INT, 0};
        const int nh = p.answers.size();

        for (const auto guessIndex: p.guesses) {
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);

            int s2 = 0, innerLb = 0, numWrongFor2 = 0;
            for (const auto answerIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                auto lbVal = 2 - (c == 1); // Assumes remdepth>=3
                innerLb += lbVal;
                s2 += 2*c - 1;
                numWrongFor2 += c > 1;
            }
            if (numWrongFor2 < minNumWrongFor2.numWrong) {
                minNumWrongFor2 = {numWrongFor2, guessIndex};
            }
            innerLb -= count[NUM_PATTERNS - 1];
            if (numWrongFor2 == 0) {
                return {0, guessIndex};
            }
            auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
            p.guesses.registerSortVec(guessIndex, sortVal);
        }

        return minNumWrongFor2;
    }

private:
    template <typename T>
    inline BestWordResult getGuessFunctionDecider(
        AnswerGuessesIndexesPair<T> &p,
        const RemDepthType remDepth,
        const int beta) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            return getDefaultBestWordResult();
        } else {
            if constexpr (isGetLowestAverage) {
                return getGuessForLowestAverage(p, remDepth, beta);
            } else {
                return getGuessForLeastWrong(p, remDepth, beta);
            }
        }
    }

    template <typename T>
    BestWordResult getGuessForLeastWrong(AnswerGuessesIndexesPair<T> &p, const RemDepthType remDepth, int beta) {
        assertm(remDepth != 0, "no tries remaining");
        if (p.answers.size() == 0) return {0, 0};
        // assumes its sorted
        //if (remDepth >= p.answers.size()) return {0, p.answers[std::min(remDepth-1, static_cast<int>(p.answers.size())-1)]};

        if (remDepth == 1) { // we can't use info from last guess
            return {
                (int)p.answers.size()-1,
                *std::min_element(p.answers.begin(), p.answers.end())
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        auto guessesRemovedByClearGuesses = 0;
        if constexpr (isEasyMode) {
            //guessesRemovedByClearGuesses += RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
            //guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
        }
        const bool shouldSort = true;
        
        BestWordResult res = getDefaultBestWordResult();
        res.wordIndex = p.answers[0];

        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(p.guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        std::array<bool, NUM_PATTERNS> patternSeen;
        std::array<int, NUM_PATTERNS> equiv;

        BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(p, equiv);

        if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
            p.guesses.restoreValues(guessesRemovedByClearGuesses);
            return setCacheVal(key, minNumWrongFor2);
        }

        if (shouldSort) p.guesses.sortBySortVec();   

        // full search for remDepth >= 3
        const int initBeta = beta;
        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            patternSeen.fill(false);
            const auto &possibleGuess = p.guesses[myInd];
            if (remDepth == maxTries) DEBUG(GlobalState.reverseIndexLookup[possibleGuess] << ": " << remDepth << ": " << getPerc(myInd, p.guesses.size()));
            int numWrongForThisGuess = 0;
            for (std::size_t i = 0; i < p.answers.size(); ++i) {
                const auto &actualWordIndex = p.answers[i];
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                if (patternSeen[patternInt]) continue;
                patternSeen[patternInt] = true;

                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, actualWordIndex, remDepth, initBeta - numWrongForThisGuess);
                const auto expNumWrongForSubtree = pr.numWrong;
                numWrongForThisGuess += expNumWrongForSubtree;
                if (numWrongForThisGuess >= beta) { numWrongForThisGuess = INF_INT; break; }
            }

            BestWordResult newRes = {numWrongForThisGuess, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                beta = std::min(beta, res.numWrong);
                if (res.numWrong == 0) {
                    if (shouldSort) restoreSort(p);
                    p.guesses.restoreValues(guessesRemovedByClearGuesses);
                    return setCacheVal(key, res);
                }
            }
        }

        if (shouldSort) restoreSort(p);
        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return res.numWrong == INF_INT ? res : setCacheVal(key, res);
    }

    int getDefaultBeta() {
        if constexpr (isGetLowestAverage) {
            return GlobalArgs.maxTotalGuesses + 1;
        } else {
            return GlobalArgs.maxWrong + 1;
        }
    }

    BestWordResult getGuessForLowestAverage(AnswerGuessesIndexesPair<UnorderedVec> &p, const RemDepthType remDepth, int beta) {
        assertm(remDepth != 0, "no tries remaining");

        if (p.answers.size() == 0) return {0, 0};
        if (p.answers.size() == 1) return {1, p.answers[0]};
        if (remDepth == 1) { // we can't use info from last guess
            return {
                INF_INT,
                p.answers[0]
            };
        }

        const int nh = p.answers.size();
        if (2*nh-1 >= beta) {
            return {2*nh-1, p.answers[0]};
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        int guessesRemovedByClearGuesses = 0;
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(p.guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        std::array<IndexType, NUM_PATTERNS> patternToRep;
        std::array<int, NUM_PATTERNS> equiv;
        IndexType good = MAX_INDEX_TYPE;

        for (const auto guessIndex: p.answers) {
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);
            int bad = 0;
            for (const auto answerIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                bad += (c>=2);
            }
            if (bad == 0) {
                p.guesses.restoreValues(guessesRemovedByClearGuesses);
                return setCacheVal(key, {2*nh-1, guessIndex});
            }
            if (bad == 1) good = guessIndex;
        }
        if (good != MAX_INDEX_TYPE && remDepth >= 3) {
            p.guesses.restoreValues(guessesRemovedByClearGuesses);
            return setCacheVal(key, {2*nh, good});
        }

        for (const auto guessIndex: p.guesses) {
            auto &lbCacheEntry = lbCache3d[remDepth][guessIndex];
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);
            lbCacheEntry.fill(0);

            int s2 = 0, innerLb = 0;
            for (const auto answerIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                auto lbVal = 2 - (c == 1); // Assumes remdepth>=3
                innerLb += lbVal;
                lbCacheEntry[patternInt] += patternInt == NUM_PATTERNS-1 ? 0 : lbVal;
                s2 += 2*c - 1;
            }
            innerLb -= count[NUM_PATTERNS - 1];
            if (count[NUM_PATTERNS - 1] == 0 && s2 == nh) {
                p.guesses.restoreValues(guessesRemovedByClearGuesses);
                return setCacheVal(key, {2*nh, guessIndex});
            }
            auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
            p.guesses.registerSortVec(guessIndex, sortVal);
        }

        // no split into singletons --> infinity
        if (remDepth <= 2) {
            p.guesses.restoreValues(guessesRemovedByClearGuesses);
            return setCacheVal(key, {INF_INT, p.answers[0]});
        }

        p.guesses.sortBySortVec();

        // begin exact for remDepth >= 3
        BestWordResult res = getDefaultBestWordResult();
        res.wordIndex = p.answers[0];

        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto possibleGuess = p.guesses[myInd];
            const auto &lbCacheEntry = lbCache3d[remDepth][possibleGuess];

            equiv.fill(0);
            int lbCacheSum = nh;

            for (const auto actualWordIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                equiv[patternInt]++;
                if (equiv[patternInt] == 1) {
                    lbCacheSum += lbCacheEntry[patternInt];
                    patternToRep[patternInt] = actualWordIndex;
                }
            }

            for (auto actualWordIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                if (patternToRep[patternInt] != actualWordIndex) continue;

                if (lbCacheSum >= beta) { lbCacheSum = INF_INT; break; }

                lbCacheSum -= lbCacheEntry[patternInt];

                const auto bestLb = std::max(0, lbCacheSum);
                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, actualWordIndex, remDepth, beta - bestLb);

                const auto numWrong = pr.numWrong;

                if (numWrong == INF_INT) { lbCacheSum = INF_INT;  break; }
                lbCacheSum += numWrong;
            }

            BestWordResult newRes = {lbCacheSum, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                beta = std::min(beta, res.numWrong);
            }
        }

        restoreSort(p);
        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return res.numWrong == INF_INT ? res : setCacheVal(key, res);
    }

    inline BestWordResult makeGuessAndRestoreAfter(
        AnswerGuessesIndexesPair<UnorderedVec> &p,
        const IndexType possibleGuess,
        const IndexType actualWordIndex,
        const RemDepthType remDepth,
        const int beta)
    {
        const auto getter = PatternGetterCached(actualWordIndex);
        const auto state = AttemptStateToUse(getter);
    
        auto numAnswersRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, p.answers);
        BestWordResult pr;
        if constexpr (isEasyMode) {
            pr = getGuessFunctionDecider(p, remDepth-1, beta);
        } else {
            auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, p.guesses);
            pr = getGuessFunctionDecider(p, remDepth-1, beta);
            p.guesses.restoreValues(numGuessesRemoved);
        }
        p.answers.restoreValues(numAnswersRemoved);
        return pr;
    }


    inline BestWordResult setCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        return getGuessCache[key] = res;
    }

    template<typename T>
    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(const AnswerGuessesIndexesPair<T> &p, const RemDepthType remDepth) {
        auto key = getCacheKey(p, remDepth);
        auto it = getGuessCache.find(key);
        if (it == getGuessCache.end()) {
            cacheMiss++;
            return {key, getDefaultBestWordResult()};
        }
        cacheHit++;
        return {key, it->second};
    }

    template<typename T>
    static AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswerGuessesIndexesPair<T> &p, RemDepthType remDepth) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, remDepth);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, p.guesses, remDepth);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF_INT, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }

    void restoreSort(AnswerGuessesIndexesPair<UnorderedVec> &p) {
        if constexpr (!isEasyMode) {
            return;
        }
        p.guesses.restoreSort();
    }
};
