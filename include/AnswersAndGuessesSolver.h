#pragma once
#include "AttemptState.h"
#include "AttemptStateFast.h"
#include "AttemptStateFaster.h"
#include "AttemptStateFastest.h"
#include "AnswersAndGuessesResult.h"

#include "PatternGetter.h"
#include "WordSetUtil.h"
#include "BestWordResult.h"
#include "AnswersAndGuessesKey.h"
#include "AnswerGuessesIndexesPair.h"
#include "Defs.h"
#include "UnorderedVector.h"
#include "RemoveGuessesBetterGuess.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) 

static constexpr double INF = 1e8;

template <bool isEasyMode, bool isGetLowestAverage=false>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(const std::vector<std::string> &allAnswers, const std::vector<std::string> &allGuesses, uint8_t maxTries, IndexType maxIncorrect)
        : allAnswers(allAnswers),
          allGuesses(allGuesses),
          reverseIndexLookup(buildLookup()),
          maxTries(maxTries),
          maxIncorrect(maxIncorrect)
        {
            precompute();
            checkWordSetSize<WordSetAnswers>("NUM_WORDS", allAnswers.size());
            checkWordSetSize<WordSetGuesses>("MAX_NUM_GUESSES", allGuesses.size());

            std::unordered_set<std::string> allGuessesSet{allGuesses.begin(), allGuesses.end()};
            for (auto ans: allAnswers) {
                if (allGuessesSet.count(ans) == 0) {
                    DEBUG("possible answer is missing in guesses: " << ans);
                    exit(1);
                }
            }
        }
    
    const std::vector<std::string> allAnswers, allGuesses;
    const std::vector<std::string> reverseIndexLookup;
    const uint8_t maxTries;
    const IndexType maxIncorrect;

    static const int isEasyModeVar = isEasyMode;

    bool skipRemoveGuessesWhichHaveBetterGuess = false;
    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, BestWordResult> getBestWordCache;
    long long cacheMiss = 0, cacheHit = 0;

    void buildStaticState() {
        buildClearGuessesInfo(reverseIndexLookup);
    }

    void setStartWord(const std::string &word) {
        DEBUG("set starting word "<< word);
        startingWord = word;
    }

    AnswersAndGuessesResult solveWord(const std::string &answer, bool getFirstWord = false) {
        if (std::find(allAnswers.begin(), allAnswers.end(), answer) == allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        IndexType firstWordIndex;

        auto p = AnswerGuessesIndexesPair<TypeToUse>(allAnswers.size(), allGuesses.size());
        if (getFirstWord) {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getBestWordDecider(p, maxTries);
            DEBUG("first word: " << reverseIndexLookup[firstPr.wordIndex] << ", known probWrong: " << firstPr.probWrong);
            firstWordIndex = firstPr.wordIndex;
        } else {
            firstWordIndex = getIndexFromStartingWord();
        }
        auto answerIndex = getAnswerIndex(answer);
        return solveWord(answerIndex, firstWordIndex, p);
    }

    IndexType getIndexFromStartingWord() const {
        return getIndexForWord(startingWord);
    }

    IndexType getIndexForWord(const std::string &word) const {
        std::size_t firstWordIndex = std::distance(
            reverseIndexLookup.begin(),
            std::find(reverseIndexLookup.begin(), reverseIndexLookup.end(), word)
        );
        if (firstWordIndex == reverseIndexLookup.size()) {
            DEBUG("guess not in word list: " << word);
            exit(1);
        }
        return (IndexType)firstWordIndex;
    }

    template<class T>
    AnswersAndGuessesResult solveWord(IndexType answerIndex, IndexType firstWordIndex, AnswerGuessesIndexesPair<T> &p) {
        AnswersAndGuessesResult res = {};
        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);

        //clearGuesses(p.guesses, p.answers);
        //removeGuessesWhichHaveBetterGuess(p, true);
        
        int guessIndex = firstWordIndex;
        for (res.tries = 1; res.tries <= maxTries; ++res.tries) {
            if (guessIndex == answerIndex) return res;
            if (res.tries == maxTries) break;

            makeGuess(p, state, guessIndex, reverseIndexLookup);

            if constexpr (std::is_same<T, UnorderedVec>::value && isEasyMode) {
                clearGuesses(p.guesses, p.answers);
                RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p, true);
            }

            GUESSESSOLVER_DEBUG(reverseIndexLookup[answerIndex] << ", " << res.tries << ": words size: " << p.answers.size() << ", guesses size: " << p.guesses.size());
            auto pr = getBestWordDecider(p, maxTries-res.tries);
            if (res.tries == 1) res.firstGuessResult = pr;
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << reverseIndexLookup[pr.wordIndex] << ", probWrong: " << pr.probWrong);

            guessIndex = pr.wordIndex;
        }
        res.tries = -1;
        return res;
    }

    IndexType getAnswerIndex(const std::string &answer) const {
        auto it = std::find(allAnswers.begin(), allAnswers.end(), answer);
        if (it == allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }
        return std::distance(allAnswers.begin(), it);
    }


    template<typename T>
    static void makeGuess(AnswerGuessesIndexesPair<T> &pair, const AttemptStateToUse &state, IndexType guessIndex, const std::vector<std::string> &reverseIndexLookup) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            pair.answers = state.guessWord(guessIndex, pair.answers, reverseIndexLookup);
            if constexpr (!isEasyMode) pair.guesses = state.guessWord(guessIndex, pair.guesses, reverseIndexLookup);
        } else {
            state.guessWordAndRemoveUnmatched(guessIndex, pair.answers);
            if constexpr (!isEasyMode) {
                state.guessWordAndRemoveUnmatched(guessIndex, pair.guesses);
            }
        }
    }

private:
    template <typename T>
    inline BestWordResult getBestWordDecider(AnswerGuessesIndexesPair<T> &p, const TriesRemainingType triesRemaining) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            return getBestWord(p, triesRemaining);
        } else {
            if constexpr (isGetLowestAverage) {
                return getWordForLowestAverage(p, triesRemaining);
            } else {
                return getBestWord2(p, triesRemaining);
            }
        }
    }

    template <typename T>
    BestWordResult getBestWord2(AnswerGuessesIndexesPair<T> &p, const TriesRemainingType triesRemaining) {
        assertm(p.answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= p.answers.size()) return {0, p.answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                ((double)(p.answers.size()-1)) / p.answers.size(),
                *std::min_element(p.answers.begin(), p.answers.end())
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        auto guessesRemovedByClearGuesses = isEasyMode ? clearGuesses(p.guesses, p.answers) : 0;
        if constexpr (std::is_same<T, UnorderedVec>::value && isEasyMode) {
            if (isEasyMode && triesRemaining >= 3) {
                guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
            }
        }
        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < p.guesses.size(); myInd++) {
            const auto &possibleGuess = p.guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, p.guesses.size()));
            double probWrong = 0.00;
            for (std::size_t i = 0; i < p.answers.size(); ++i) {
                const auto &actualWordIndex = p.answers[i];
                const auto getter = PatternGetterCached(actualWordIndex);
                const auto state = AttemptStateToUse(getter);

                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, triesRemaining, state);
                probWrong += pr.probWrong;
                if (pr.probWrong * (p.answers.size() - numAnswersRemoved) > maxIncorrect+0.1) {
                    probWrong = 2 * p.answers.size();
                    break;
                }
            }

            BestWordResult newRes = {probWrong / p.answers.size(), possibleGuess};
            if (newRes.probWrong < res.probWrong || (newRes.probWrong == res.probWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                if (res.probWrong == 0.00) {
                    p.guesses.restoreValues(guessesRemovedByClearGuesses);
                    return setCacheVal(key, res);
                }
            }
        }

        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    BestWordResult getWordForLowestAverage(AnswerGuessesIndexesPair<UnorderedVec> &p, const TriesRemainingType triesRemaining) {
        assertm(p.answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (p.answers.size() == 1) return {1.00, p.answers[0]};
        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                INF,
                p.answers[0]
            };
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(p, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        auto guessesRemovedByClearGuesses = isEasyMode ? clearGuesses(p.guesses, p.answers) : 0;

        if (isEasyMode && triesRemaining >= 3) {
            guessesRemovedByClearGuesses += RemoveGuessesBetterGuess::removeGuessesWhichHaveBetterGuess(p);
        }

        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < p.guesses.size(); myInd++) {
            const auto &possibleGuess = p.guesses[myInd];            
            double sumOfAveragesForThisGuess = 0.00;
            for (std::size_t i = 0; i < p.answers.size(); ++i) {
                const auto &actualWordIndex = p.answers[i];
                auto getter = PatternGetterCached(actualWordIndex);
                auto state = AttemptStateToUse(getter);

                auto pr = makeGuessAndRestoreAfter(p, possibleGuess, triesRemaining, state);
                sumOfAveragesForThisGuess += pr.probWrong;
                if (pr.probWrong == INF) {
                    sumOfAveragesForThisGuess = INF;
                    break;
                }
            }
            double averageForThisGuess = 1.00 + ((sumOfAveragesForThisGuess - 1.00) / p.answers.size());

            BestWordResult newRes = {averageForThisGuess, possibleGuess};
            if (newRes.probWrong < res.probWrong || (newRes.probWrong == res.probWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
            }
        }

        p.guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    static inline thread_local std::size_t numAnswersRemoved;

    inline BestWordResult makeGuessAndRestoreAfter(
        AnswerGuessesIndexesPair<UnorderedVec> &p,
        const IndexType possibleGuess,
        const TriesRemainingType triesRemaining,
        const AttemptStateToUse &state)
    {
        auto nR = state.guessWordAndRemoveUnmatched(possibleGuess, p.answers);
        BestWordResult pr;
        if constexpr (isEasyMode) {
            pr = getBestWordDecider(p, triesRemaining-1);
        } else {
            auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, p.guesses);
            pr = getBestWordDecider(p, triesRemaining-1);
            p.guesses.restoreValues(numGuessesRemoved);
        }
        if constexpr (!isGetLowestAverage) {
            numAnswersRemoved = nR;
        }
        p.answers.restoreValues(nR);
        return pr;
    }

    BestWordResult getBestWord(AnswerGuessesIndexesPair<std::vector<IndexType>> &p, const TriesRemainingType triesRemaining) {
        assertm(p.answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= p.answers.size()) return BestWordResult {0.00, p.answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {((double)(p.answers.size()-1)) / p.answers.size(), p.answers[0]};
        }

        std::vector<IndexType> clearedGuesses;
        const auto &answers = p.answers;
        const auto &guesses = isEasyMode
            ? (clearedGuesses = clearGuesses(p.guesses, p.answers))
            : p.guesses;

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) return cacheVal;

        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            double probWrong = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetter(reverseIndexLookup[actualWordIndex]);
                auto state = AttemptStateToUse(getter);

                const auto answerWords = state.guessWord(possibleGuess, answers, reverseIndexLookup);
                BestWordResult pr;
                if constexpr (isEasyMode) {
                    pr = getBestWordDecider({answerWords, guesses}, triesRemaining-1);
                } else {
                    const auto nextGuessWords = state.guessWord(possibleGuess, guesses, reverseIndexLookup);
                    pr = getBestWordDecider({answerWords, nextGuessWords}, triesRemaining-1);
                }
                probWrong += pr.probWrong;
                if (pr.probWrong * answerWords.size() > maxIncorrect+0.1) {
                    probWrong = 2 * answers.size();
                    break;
                }
            }

            BestWordResult newRes = {probWrong / answers.size(), possibleGuess};
            if (newRes.probWrong < res.probWrong) {
                res = newRes;
                if (res.probWrong == 0) {
                    return setCacheVal(key, res);
                }
            }
        }

        return setCacheVal(key, res);
    }

    void precompute() {
        GUESSESSOLVER_DEBUG("precompute AnswersAndGuessesSolver.");
        getBestWordCache = {};
    }

    inline std::vector<IndexType> clearGuesses(std::vector<IndexType> guesses, const std::vector<IndexType> &answers) {   
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        std::erase_if(guesses, [&](const int &guessIndex) {
            return (answerLetterMask & letterCountLookup[guessIndex]) == 0;
        });
        return guesses;
    }

    std::size_t clearGuesses(UnorderedVector<IndexType> &guesses, const UnorderedVector<IndexType> &answers) {   
        std::size_t guessesRemoved = 0;
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        for (std::size_t i = guesses.size()-1; i != MAX_SIZE_VAL; --i) {
            auto guessIndex = guesses[i];
            if ((answerLetterMask & letterCountLookup[guessIndex]) == 0) {
                guesses.deleteIndex(i);
                guessesRemoved++;
            }
        }

        return guessesRemoved;
    }

    inline static std::vector<int> letterCountLookup = {};
    static void buildClearGuessesInfo(const std::vector<std::string> &reverseIndexLookup) {
        if (letterCountLookup.size() > 0) return;
        letterCountLookup.resize(reverseIndexLookup.size());
        for (std::size_t i = 0; i < reverseIndexLookup.size(); ++i) {
            letterCountLookup[i] = 0;
            for (auto c: reverseIndexLookup[i]) {
                letterCountLookup[i] |= (1 << (c-'a'));
            }
        }
    }

    // hash table
    auto buildLookup() {
        std::vector<std::string> lookup(allGuesses.size());

        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            lookup[i] = allGuesses[i];
            if (i < allAnswers.size() && allGuesses[i] != allAnswers[i]) {
                DEBUG("allGuesses[" << i << "] should equal allAnswers[" << i << "], " << allGuesses[i] << " vs " << allAnswers[i]);
                exit(1);
            }
        }

        return lookup;
    }

    const BestWordResult getFromCache(const AnswersAndGuessesKey<isEasyMode> &key) {
        auto it = getBestWordCache.find(key);
        if (it == getBestWordCache.end()) {
            cacheMiss++;
            return getDefaultBestWordResult();
        }

        cacheHit++;
        return it->second;
    }

    inline BestWordResult setCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        return getBestWordCache[key] = res;
    }

    template<typename T>
    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(const AnswerGuessesIndexesPair<T> &p, const TriesRemainingType triesRemaining) {
        auto key = getCacheKey(p, triesRemaining);
        const auto cacheVal = getFromCache(key);
        return {key, cacheVal};
    }

public:
    template<typename T>
    static AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswerGuessesIndexesPair<T> &p, TriesRemainingType triesRemaining) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, triesRemaining);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(p.answers, p.guesses, triesRemaining);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }
};
