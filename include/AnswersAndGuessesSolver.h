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
    const bool useExactSearch = true; // only if probablity is 1.00

    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey, BestWordResult> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;
    long long easierThanProblemCacheHit = 0, easierThanProblemCacheMiss = 0;

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
            // makeGuessAndRemoveUnmatched
            state.guessWordAndRemoveUnmatched(guessIndex, pair.answers, reverseIndexLookup);
            if constexpr (!isEasyMode) {
                state.guessWordAndRemoveUnmatched(guessIndex, pair.guesses, reverseIndexLookup);
            }
        }
    }

private:
    template<typename T>
    BestWordResult getBestWordDecider(T& answers, T& guesses, uint8_t triesRemaining) {
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

    BestWordResult getBestWord2(UnorderedVector<IndexType> &answers, UnorderedVector<IndexType> &guesses, uint8_t triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= answers.size()) return {0, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {
                ((double)(answers.size()-1)) / answers.size(),
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
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            double probWrong = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetterCached(actualWordIndex);
                auto state = AttemptStateToUse(getter);

                auto pr = makeGuessAndRestoreAfter(answers, guesses, possibleGuess, triesRemaining, reverseIndexLookup, state);
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

    BestWordResult getWordForLowestAverage(UnorderedVector<IndexType> &answers, UnorderedVector<IndexType> &guesses, uint8_t triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining == 1) { // we can't use info from last guess
            if (answers.size() == 1) return {1.00, answers[0]};
            return {INF, answers[0]};
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

                auto pr = makeGuessAndRestoreAfter(answers, guesses, possibleGuess, triesRemaining, reverseIndexLookup, state);
                sumOfAveragesForThisGuess += pr.probWrong;
            }
            double averageForThisGuess = 1.00 + (sumOfAveragesForThisGuess / answers.size());

            BestWordResult newRes = {averageForThisGuess, possibleGuess};
            if (newRes.probWrong < res.probWrong || (newRes.probWrong == res.probWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
            }
        }

        if (res.probWrong > 5.00 && res.probWrong < 100) DEBUG("EXP AVG: " << res.probWrong);

        guesses.restoreValues(guessesRemovedByClearGuesses);
        return setCacheVal(key, res);
    }

    static inline thread_local std::size_t numAnswersRemoved;

    BestWordResult makeGuessAndRestoreAfter(
        UnorderedVector<IndexType> &answers,
        UnorderedVector<IndexType> &guesses,
        IndexType possibleGuess,
        uint8_t triesRemaining,
        const std::vector<std::string> &reverseIndexLookup,
        const AttemptStateToUse &state)
    {
        auto nR = state.guessWordAndRemoveUnmatched(possibleGuess, answers, reverseIndexLookup);
        BestWordResult pr;
        if constexpr (isEasyMode) {
            pr = getBestWordDecider(answers, guesses, triesRemaining-1);
        } else {
            auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, guesses, reverseIndexLookup);
            //DEBUG((int)triesRemaining << ": REMOVED " << numGuessesRemoved << " GUESSES" << ", size:" << guesses.size() << " maxsize: " << guesses.maxSize);
            pr = getBestWordDecider(answers, guesses, triesRemaining-1);
            //DEBUG((int)triesRemaining << ": RESTORE " << numGuessesRemoved << " GUESSES");
            guesses.restoreValues(numGuessesRemoved);
        }
        numAnswersRemoved = nR;
        answers.restoreValues(numAnswersRemoved);
        return pr;
    }

    BestWordResult getBestWord(const std::vector<IndexType> &answers, const std::vector<IndexType> &_guesses, uint8_t triesRemaining) {
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

    static AnswersAndGuessesKey getCacheKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, uint8_t triesRemaining) {
        return AnswersAndGuessesKey(wsAnswers, wsGuesses, triesRemaining);
    }

    const BestWordResult& getFromCache(const AnswersAndGuessesKey &key) {
        auto it = getBestWordCache.find(key);
        if (it == getBestWordCache.end()) {
            cacheMiss++;
            if constexpr (false && !isGetLowestAverage) {
                if (key.triesRemaining == GlobalArgs.maxTries - 1) {
                    //DEBUG("CHECKING....");
                    for (auto it = getBestWordCache.cbegin(); it != getBestWordCache.cend(); ++it) {
                        if (it->second.probWrong == 0.00 && key.isEasierThanProblem(it->first)) {
                            easierThanProblemCacheHit++;
                            //DEBUG("HIT");
                            return it->second;
                        }
                    }
                    easierThanProblemCacheMiss++;
                }
            }
            return getDefaultBestWordResult();
        }

        cacheHit++;
        return it->second;
    }

    inline BestWordResult setCacheVal(const AnswersAndGuessesKey &key, const BestWordResult &res) {
        if (getBestWordCache.count(key) == 0) {
            cacheSize++;
            //if (key.triesRemaining == maxTries) return res;
            getBestWordCache[key] = res;
        }
        return getBestWordCache[key];
    }

    template<typename T>
    std::pair<AnswersAndGuessesKey, const BestWordResult> getCacheKeyAndValue(T& answers, T& guesses, uint8_t triesRemaining) {
        auto key = getCacheKey(answers, guesses, triesRemaining);
        const auto &cacheVal = getFromCache(key);
        return {key, cacheVal};
    }

public:
    template<typename T>
    static AnswersAndGuessesKey getCacheKey(T& answers, T& guesses, uint8_t triesRemaining) {
        auto wsAnswers = buildAnswersWordSet(answers);
        auto wsGuesses = buildGuessesWordSet(guesses);
        return getCacheKey(wsAnswers, wsGuesses, triesRemaining);
    }

    template<typename Vec, typename T>
    static T buildWordSet(const Vec &wordIndexes, int offset) {
        auto wordset = T();
        for (auto wordIndex: wordIndexes) {
            wordset[offset + wordIndex] = true;
        }
        return wordset;
    }

    template<typename Vec>
    static WordSetAnswers buildAnswersWordSet(const Vec &answers) {
        return buildWordSet<Vec, WordSetAnswers>(answers, 0);
    }

    template<typename Vec>
    static WordSetGuesses buildGuessesWordSet(const Vec &guesses) {
        return buildWordSet<Vec, WordSetGuesses>(guesses, 0);
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }

};
