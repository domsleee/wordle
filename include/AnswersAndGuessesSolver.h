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
#include "AnswerGuessesIndexPair.h"
#include "Defs.h"
#include "UnorderedVector.h"

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
            auto answersSize = WordSetAnswers().size();
            if (allAnswers.size() > NUM_WORDS) {
                DEBUG("ERROR: answers too big " << allAnswers.size() << " vs " << answersSize);
                exit(1);
            }
            auto guessesSize = WordSetGuesses().size();
            if (allGuesses.size() > guessesSize) {
                DEBUG("ERROR: guesses too big " << allGuesses.size() << " vs " << guessesSize);
                exit(1);
            }

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

        auto p = AnswersGuessesIndexesPair<TypeToUse>(allAnswers.size(), allGuesses.size());
        if (getFirstWord) {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getBestWordDecider(p.answers, p.guesses, maxTries);
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
    AnswersAndGuessesResult solveWord(IndexType answerIndex, IndexType firstWordIndex, AnswersGuessesIndexesPair<T> &p) {
        AnswersAndGuessesResult res = {};
        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);
        
        int guessIndex = firstWordIndex;
        for (res.tries = 1; res.tries <= maxTries; ++res.tries) {
            if (guessIndex == answerIndex) return res;
            if (res.tries == maxTries) break;

            makeGuess(p, state, guessIndex, reverseIndexLookup);

            GUESSESSOLVER_DEBUG(reverseIndexLookup[answerIndex] << ", " << res.tries << ": words size: " << p.answers.size() << ", guesses size: " << p.guesses.size());
            auto pr = getBestWordDecider(p.answers, p.guesses, maxTries-res.tries);
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
    static void makeGuess(AnswersGuessesIndexesPair<T> &pair, const AttemptStateToUse &state, IndexType guessIndex, const std::vector<std::string> &reverseIndexLookup) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            pair.answers = state.guessWord(guessIndex, pair.answers, reverseIndexLookup);
            if constexpr (!isEasyMode) pair.guesses = state.guessWord(guessIndex, pair.guesses, reverseIndexLookup);
        } else {
            state.guessWordAndRemoveUnmatched(guessIndex, pair.answers, reverseIndexLookup);
            if constexpr (!isEasyMode) {
                state.guessWordAndRemoveUnmatched(guessIndex, pair.guesses, reverseIndexLookup);
            }
        }
    }

private:

    /*inline BestWordResult getBestWordDecider(std::vector<IndexType>& answers, std::vector<IndexType>& guesses, TriesRemainingType triesRemaining) {
        return getBestWord(answers, guesses, triesRemaining);
    }

    template <typename ...Params>
    inline BestWordResult getBestWordDecider(Params&&... params)
    {
        if constexpr (isGetLowestAverage) {
            return getWordForLowestAverage(std::forward<Params>(params)...);
        } else {
            return getBestWord2(std::forward<Params>(params)...);
        }
    }*/
    
    template <typename T>
    inline BestWordResult getBestWordDecider(T& answers, T& guesses, const TriesRemainingType triesRemaining) {
        if constexpr (std::is_same<T, std::vector<IndexType>>::value) {
            return getBestWord(answers, guesses, triesRemaining);
        } else {
            if constexpr (isGetLowestAverage) {
                return getWordForLowestAverage(answers, guesses, triesRemaining);
            } else {
                return getBestWord2(answers, guesses, triesRemaining);
            }
        }
    }

    BestWordResult getBestWord2(UnorderedVector<IndexType> &answers, UnorderedVector<IndexType> &guesses, const TriesRemainingType triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= answers.size()) return {0, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                ((double)(answers.size()-1)) / answers.size(),
                *std::min_element(answers.begin(), answers.end())
            };
        }

        const auto guessesRemovedByClearGuesses = isEasyMode ? clearGuesses(guesses, answers) : 0;

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            guesses.restoreValues(guessesRemovedByClearGuesses);
            return cacheVal;
        }

        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            double probWrong = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                const auto getter = PatternGetterCached(actualWordIndex);
                const auto state = AttemptStateToUse(getter);

                const auto pr = makeGuessAndRestoreAfter(answers, guesses, possibleGuess, triesRemaining, state);
                probWrong += pr.probWrong;
                if (pr.probWrong * (answers.size() - numAnswersRemoved) > maxIncorrect+0.1) {
                    probWrong = 2 * answers.size();
                    break;
                }
            }

            BestWordResult newRes = {probWrong / answers.size(), possibleGuess};
            if (newRes.probWrong < res.probWrong || (newRes.probWrong == res.probWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                if (res.probWrong == 0.00) {
                    guesses.restoreValues(guessesRemovedByClearGuesses);
                    return setCacheVal(key, res);
                }
            }
        }

        guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    BestWordResult getWordForLowestAverage(UnorderedVector<IndexType> &answers, UnorderedVector<IndexType> &guesses, const TriesRemainingType triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (answers.size() == 1) return {1.00, answers[0]};
        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                INF,
                *std::min_element(answers.begin(), answers.end())
            };
        }

        auto guessesRemovedByClearGuesses = isEasyMode ? clearGuesses(guesses, answers) : 0;

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, triesRemaining);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            guesses.restoreValues(guessesRemovedByClearGuesses);
            return cacheVal;
        }

        BestWordResult res = getDefaultBestWordResult();
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];            
            double sumOfAveragesForThisGuess = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetterCached(actualWordIndex);
                auto state = AttemptStateToUse(getter);

                auto pr = makeGuessAndRestoreAfter(answers, guesses, possibleGuess, triesRemaining, state);
                sumOfAveragesForThisGuess += pr.probWrong;
            }
            double averageForThisGuess = 1.00 + ((sumOfAveragesForThisGuess - 1.00) / answers.size());

            BestWordResult newRes = {averageForThisGuess, possibleGuess};
            if (newRes.probWrong < res.probWrong || (newRes.probWrong == res.probWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
            }
        }

        guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    static inline thread_local std::size_t numAnswersRemoved;

    inline BestWordResult makeGuessAndRestoreAfter(
        UnorderedVector<IndexType> &answers,
        UnorderedVector<IndexType> &guesses,
        const IndexType possibleGuess,
        const TriesRemainingType triesRemaining,
        const AttemptStateToUse &state)
    {
        auto nR = state.guessWordAndRemoveUnmatched(possibleGuess, answers, reverseIndexLookup);
        BestWordResult pr;
        if constexpr (isEasyMode) {
            pr = getBestWordDecider(answers, guesses, triesRemaining-1);
        } else {
            auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, guesses, reverseIndexLookup);
            pr = getBestWordDecider(answers, guesses, triesRemaining-1);
            guesses.restoreValues(numGuessesRemoved);
        }
        if constexpr (!isGetLowestAverage) {
            numAnswersRemoved = nR;
        }
        answers.restoreValues(nR);
        return pr;
    }

    BestWordResult getBestWord(const std::vector<IndexType> &answers, const std::vector<IndexType> &_guesses, const TriesRemainingType triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= answers.size()) return BestWordResult {0.00, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {((double)(answers.size()-1)) / answers.size(), answers[0]};
        }

        std::vector<IndexType> clearedGuesses;
        const auto &guesses = isEasyMode
            ? (clearedGuesses = clearGuesses(_guesses, answers))
            : _guesses;

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
                    pr = getBestWordDecider(answerWords, guesses, triesRemaining-1);
                } else {
                    const auto nextGuessWords = state.guessWord(possibleGuess, guesses, reverseIndexLookup);
                    pr = getBestWordDecider(answerWords, nextGuessWords, triesRemaining-1);
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
    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(T& answers, T& guesses, const TriesRemainingType triesRemaining) {
        auto key = getCacheKey(answers, guesses, triesRemaining);
        const auto cacheVal = getFromCache(key);
        return {key, cacheVal};
    }

public:
    template<typename T>
    static AnswersAndGuessesKey<isEasyMode> getCacheKey(const T& answers, const T& guesses, TriesRemainingType triesRemaining) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(answers, triesRemaining);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(answers, guesses, triesRemaining);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }

};
