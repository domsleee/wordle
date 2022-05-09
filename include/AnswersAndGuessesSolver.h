#pragma once
#include "AttemptState.h"
#include "AttemptStateFaster.h"
#include "AnswersAndGuessesResult.h"
#include "GlobalState.h"

#include "PatternGetter.h"
#include "WordSetUtil.h"
#include "BestWordResult.h"
#include "AnswersAndGuessesKey.h"
#include "Defs.h"
#include "UnorderedVector.h"
#include "RemoveGuessesWithNoLetterInAnswers.h"

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
    std::vector<std::array<AnswersVec, NUM_PATTERNS>> myEquivCache;

    static const int isEasyModeVar = isEasyMode;

    AnswersAndGuessesResult solveWord(const std::string &answer, std::shared_ptr<SolutionModel> solutionModel) {
        if (std::find(GlobalState.allAnswers.begin(), GlobalState.allAnswers.end(), answer) == GlobalState.allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        IndexType firstWordIndex;

        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        if (startingWord == "") {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getGuessFunctionDecider(answers, guesses, maxTries, getDefaultBeta());
            DEBUG("first word: " << GlobalState.reverseIndexLookup[firstPr.wordIndex] << ", known numWrong: " << firstPr.numWrong);
            firstWordIndex = firstPr.wordIndex;
        } else {
            firstWordIndex = GlobalState.getIndexForWord(startingWord);
        }
        auto answerIndex = GlobalState.getIndexForWord(answer);
        return solveWord(answerIndex, solutionModel, firstWordIndex, answers, guesses);
    }

    AnswersAndGuessesResult solveWord(IndexType answerIndex, std::shared_ptr<SolutionModel> solutionModel, IndexType firstWordIndex, AnswersVec &answers, GuessesVec &guesses) {
        if (lbCache3d.size() == 0 && isGetLowestAverage) lbCache3d.resize(10);
        if (myEquivCache.size() == 0) {
            myEquivCache.assign(10, {});
            for (int remDepth = 0; remDepth < 10; ++remDepth) {
                for (std::size_t i = 0; i < NUM_PATTERNS; ++i) {
                    myEquivCache[remDepth][i].reserve(NUM_WORDS);
                }
            }
        }
        AnswersAndGuessesResult res = {};
        res.solutionModel = solutionModel;

        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);
        auto currentModel = res.solutionModel;

        //RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
        //RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p, true);
        
        IndexType guessIndex = firstWordIndex;
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

            makeGuess(answers, guesses, state, guessIndex);
            /*auto it = std::find(answers.begin(), answers.end(), answerIndex);
            if (it == answers.end()) {
                DEBUG("the actual answer " << answerIndex << " was removed from answers after guess " << guessIndex << "!"); exit(1);
            }*/
            //DEBUG("result:")

            if constexpr (isEasyMode) {
                //RemoveGuessesWithNoLetterInAnswers::clearGuesses(p.guesses, p.answers);
            }

            auto pr = getGuessFunctionDecider(answers, guesses, maxTries-res.tries, getDefaultBeta());
            if (res.tries == 1) res.firstGuessResult = pr;
            const auto patternInt = getter.getPatternIntCached(guessIndex);

            GUESSESSOLVER_DEBUG("NEXT GUESS: " << GlobalState.reverseIndexLookup[pr.wordIndex] <<  ",patternInt: " << patternInt << ", numWrong: " << pr.numWrong << " numAnswers: " << answers.size());

            const auto patternStr = PatternIntHelpers::patternIntToString(patternInt);
            currentModel = currentModel->getOrCreateNext(patternStr);

            guessIndex = pr.wordIndex;
        }
        res.tries = TRIES_FAILED;
        return res;
    }


    void makeGuess(AnswersVec &answers, GuessesVec &guesses, const AttemptStateToUse &state, IndexType guessIndex) const {
        const auto myGuessPatternInt = state.patternGetter.getPatternIntCached(guessIndex);
        std::erase_if(answers, [&](const auto answerIndex) {
            return PatternGetterCached::getPatternIntCached(answerIndex, guessIndex) != myGuessPatternInt;
        });
        /*
        std::size_t ind = 0;
        for (auto answerIndex: answers) {
            const auto patternInt = state.patternGetter.getPatternIntCached(answerIndex);
            if (patternInt == myGuessPatternInt) {
                answers[ind++] = answerIndex;
            }
        }
        answers.resize(ind);*/
        if constexpr (!isEasyMode) {
            for (std::size_t i = 0; i < answers.size(); ++i) guesses[i] = answers[i];
            guesses.resize(answers.size());
        }
    }

    static BestWordResult calcSortVectorAndGetMinNumWrongFor2(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        std::array<int, NUM_PATTERNS> &equiv,
        std::array<int64_t, MAX_NUM_GUESSES> &sortVec)
    {
        BestWordResult minNumWrongFor2 = {INF_INT, 0};
        const int nh = answers.size();

        for (const auto guessIndex: guesses) {
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);

            int s2 = 0, innerLb = 0, numWrongFor2 = 0;
            for (const auto answerIndex: answers) {
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
            sortVec[guessIndex] = sortVal;
        }

        return minNumWrongFor2;
    }

private:
    inline BestWordResult getGuessFunctionDecider(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        const int beta) {
        if constexpr (isGetLowestAverage) {
            return getGuessForLowestAverage(answers, guesses, remDepth, beta);
        } else {
            return getGuessForLeastWrong(answers, guesses, remDepth, beta);
        }
    }

    BestWordResult getGuessForLeastWrong(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, int beta) {
        assertm(remDepth != 0, "no tries remaining");
        
        if (answers.size() == 0) return {0, 0};
        // assumes its sorted
        //if (remDepth >= answers.size()) return {0, answers[std::min(remDepth-1, static_cast<int>(answers.size())-1)]};

        if (remDepth == 1) { // we can't use info from last guess
            return {
                (int)answers.size()-1,
                *std::min_element(answers.begin(), answers.end())
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        if constexpr (isEasyMode) {
            //RemoveGuessesWithNoLetterInAnswers::clearGuesses(guesses, answers);
            //guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
        }

        std::array<int, NUM_PATTERNS> equiv;
        std::array<int64_t, MAX_NUM_GUESSES> sortVec;
        BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(answers, guesses, equiv, sortVec);
        if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
            return setCacheVal(key, minNumWrongFor2);
        }

        const bool shouldSort = true;
        auto guessesCopy = guesses;
        if (shouldSort) std::sort(guessesCopy.begin(), guessesCopy.end(), [&](IndexType a, IndexType b) { return sortVec[a] < sortVec[b]; });

        // full search for remDepth >= 3
        std::array<AnswersVec, NUM_PATTERNS> &myEquiv = myEquivCache[remDepth];
        BestWordResult res = getDefaultBestWordResult();
        res.wordIndex = answers[0];

        std::size_t numGuessIndexesToCheck = std::min(guessesCopy.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        const int initBeta = beta;
        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto possibleGuess = guessesCopy[myInd];
            int numWrongForThisGuess = 0;
            
            for (std::size_t i = 0; i < NUM_PATTERNS; ++i) myEquiv[i].resize(0);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, possibleGuess);
                myEquiv[patternInt].push_back(answerIndex);
            }

            for (std::size_t i = 0; i < NUM_PATTERNS-1; ++i) {
                if (myEquiv[i].size() == 0) continue;
                const auto pr = makeGuessAndRestoreAfter(myEquiv[i], guessesCopy, remDepth, initBeta - numWrongForThisGuess);
                const auto expNumWrongForSubtree = pr.numWrong;
                numWrongForThisGuess += expNumWrongForSubtree;
                if (numWrongForThisGuess >= beta) { numWrongForThisGuess = INF_INT; break; }
            }

            BestWordResult newRes = {numWrongForThisGuess, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                beta = std::min(beta, res.numWrong);
                if (res.numWrong == 0) {
                    //if (remDepth == 4) DEBUG("chosen at 4 " << res.wordIndex << " " << GlobalState.reverseIndexLookup[res.wordIndex] << " index: " << getFrac(myInd, numGuessIndexesToCheck) << " 118: " << myEquiv[118].size() << " considering #answers: " << answers.size());
                    //if (remDepth >= 4) DEBUG("chosen at " << (int)remDepth << ": " << GlobalState.reverseIndexLookup[res.wordIndex] << " " << getFrac(myInd, numGuessIndexesToCheck));
                    return setCacheVal(key, res);
                }
            }
        }

        return res.numWrong == INF_INT ? res : setCacheVal(key, res);
    }

    int getDefaultBeta() {
        if constexpr (isGetLowestAverage) {
            return GlobalArgs.maxTotalGuesses + 1;
        } else {
            return GlobalArgs.maxWrong + 1;
        }
    }

    BestWordResult getGuessForLowestAverage(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, int beta) {
        assertm(remDepth != 0, "no tries remaining");

        return {0, 0};

        /*

        if (answers.size() == 0) return {0, 0};
        if (answers.size() == 1) return {1, answers[0]};
        if (remDepth == 1) { // we can't use info from last guess
            return {
                INF_INT,
                answers[0]
            };
        }

        const int nh = answers.size();
        if (2*nh-1 >= beta) {
            return {2*nh-1, answers[0]};
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        int guessesRemovedByClearGuesses = 0;
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        std::array<IndexType, NUM_PATTERNS> patternToRep;
        std::array<int, NUM_PATTERNS> equiv;
        IndexType good = MAX_INDEX_TYPE;

        for (const auto guessIndex: answers) {
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);
            int bad = 0;
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                bad += (c>=2);
            }
            if (bad == 0) {
                return setCacheVal(key, {2*nh-1, guessIndex});
            }
            if (bad == 1) good = guessIndex;
        }
        if (good != MAX_INDEX_TYPE && remDepth >= 3) {
            return setCacheVal(key, {2*nh, good});
        }

        for (const auto guessIndex: guesses) {
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
            return setCacheVal(key, {INF_INT, p.answers[0]});
        }

        guesses.sortBySortVec();

        // begin exact for remDepth >= 3
        BestWordResult res = getDefaultBestWordResult();
        res.wordIndex = answers[0];

        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto possibleGuess = guesses[myInd];
            const auto &lbCacheEntry = lbCache3d[remDepth][possibleGuess];

            equiv.fill(0);
            int lbCacheSum = nh;

            for (const auto actualWordIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                equiv[patternInt]++;
                if (equiv[patternInt] == 1) {
                    lbCacheSum += lbCacheEntry[patternInt];
                    patternToRep[patternInt] = actualWordIndex;
                }
            }

            for (auto actualWordIndex: answers) {
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

        return res.numWrong == INF_INT ? res : setCacheVal(key, res);*/
    }

    inline BestWordResult makeGuessAndRestoreAfter(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        const int beta)
    {
        if constexpr (isEasyMode) {
            return getGuessFunctionDecider(answers, guesses, remDepth-1, beta);
        } else {
            //auto guesses2 = guesses; // todo
           return getGuessFunctionDecider(answers, guesses, remDepth-1, beta);
        }
    }


    inline BestWordResult setCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        return getGuessCache[key] = res;
    }

    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth) {
        auto key = getCacheKey(answers, guesses, remDepth);
        auto it = getGuessCache.find(key);
        if (it == getGuessCache.end()) {
            cacheMiss++;
            return {key, getDefaultBestWordResult()};
        }
        cacheHit++;
        return {key, it->second};
    }

    static AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswersVec &answers, const GuessesVec &guesses, RemDepthType remDepth) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(answers, remDepth);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(answers, guesses, remDepth);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF_INT, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }
};
