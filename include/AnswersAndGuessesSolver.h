#pragma once
#include "AttemptState.h"
#include "AttemptStateFast.h"
#include "AttemptStateFaster.h"
#include "AttemptStateFastest.h"

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

// m1pro: --max-tries 3 --max-incorrect 50 -r
// 52s: std::vector<IndexType>
// 28s: UnorderedVector<IndexType>
using TypeToUse = UnorderedVector<IndexType>;

template <bool isEasyMode>
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

    std::string startingWord = "blahs";
    std::unordered_map<AnswersAndGuessesKey, BestWordResult> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;

    void setStartWord(const std::string &word) {
        DEBUG("set starting word "<< word);
        startingWord = word;
    }

    int solveWord(const std::string &answer, bool getFirstWord = false) {
        if (std::find(allAnswers.begin(), allAnswers.end(), answer) == allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        int firstWordIndex;

        auto p = AnswersGuessesIndexesPair<TypeToUse>(allAnswers.size(), allGuesses.size());
        if (getFirstWord) {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getBestWordDecider(p.answers, p.guesses, maxTries);
            DEBUG("first word: " << reverseIndexLookup[firstPr.wordIndex] << ", known prob: " << firstPr.prob);
            firstWordIndex = firstPr.wordIndex;
            if (useExactSearch && firstPr.prob != 1.00) return -1;
        } else {
            firstWordIndex = std::distance(
                reverseIndexLookup.begin(),
                std::find(reverseIndexLookup.begin(), reverseIndexLookup.end(), startingWord)
            );
            if (firstWordIndex == reverseIndexLookup.size()) {
                DEBUG("guess not in word list: " << startingWord);
                exit(1);
            }
        }

        return solveWord(answer, firstWordIndex, p);
    }

    template<class T>
    int solveWord(const std::string &answer, int firstWordIndex, AnswersGuessesIndexesPair<T> &p) {
        auto getter = PatternGetter(answer);
        auto state = AttemptStateToUse(getter);
        
        int guessIndex = firstWordIndex;
        for (int tries = 1; tries <= maxTries; ++tries) {
            if (reverseIndexLookup[guessIndex] == answer) return tries;
            if (tries == maxTries) break;

            makeGuess(p, state, guessIndex, reverseIndexLookup);

            GUESSESSOLVER_DEBUG(answer << ", " << tries << ": words size: " << answerIndexes.size() << ", guesses size: " << guessIndexes.size());
            auto pr = getBestWordDecider(p.answers, p.guesses, maxTries-tries);
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << reverseIndexLookup[pr.wordIndex] << ", PROB: " << pr.prob);

            if (EARLY_EXIT && pr.prob != 1.00) break;

            guessIndex = pr.wordIndex;
        }
        return -1;
    }


    template<typename T>
    void makeGuess(AnswersGuessesIndexesPair<T> &pair, const AttemptStateToUse &state, IndexType guessIndex, const std::vector<std::string> &reverseIndexLookup) {
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
            return getBestWord2(answers, guesses, triesRemaining);
        }
    }

    
    BestWordResult getBestWord2(UnorderedVector<IndexType> &answers, UnorderedVector<IndexType> &guesses, uint8_t triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= answers.size()) return BestWordResult {1.00, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {1.00/answers.size(), answers[0]};
        }

        /*std::vector<IndexType> clearedGuesses;
        const auto &guesses = isEasyMode
            ? (clearedGuesses = clearGuesses(_guesses, answers))
            : _guesses;*/

        auto wsAnswers = buildAnswersWordSet(answers);
        auto wsGuesses = buildGuessesWordSet(guesses);
        auto key = getCacheKey(wsAnswers, wsGuesses, triesRemaining);

        auto cacheVal = getFromCache(key);
        if (cacheVal.prob != -1) return cacheVal;

        BestWordResult res = {-1.00, -1};
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            auto prob = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetter(reverseIndexLookup[actualWordIndex]);
                auto state = AttemptStateToUse(getter);

                auto numAnswersRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, answers, reverseIndexLookup);
                BestWordResult pr;
                if constexpr (isEasyMode) {
                    pr = getBestWord2(answers, guesses, triesRemaining-1);
                } else {
                    auto numGuessesRemoved = state.guessWordAndRemoveUnmatched(possibleGuess, guesses, reverseIndexLookup);
                    //DEBUG((int)triesRemaining << ": REMOVED " << numGuessesRemoved << " GUESSES" << ", size:" << guesses.size() << " maxsize: " << guesses.maxSize);
                    pr = getBestWord2(answers, guesses, triesRemaining-1);
                    //DEBUG((int)triesRemaining << ": RESTORE " << numGuessesRemoved << " GUESSES");
                    guesses.restoreValues(numGuessesRemoved);
                }
                answers.restoreValues(numAnswersRemoved);
                prob += pr.prob;
                if (EARLY_EXIT && pr.prob != 1) break;
                else if ((1.00-pr.prob) * answers.size() > maxIncorrect) break;
            }

            BestWordResult newRes = {prob/answers.size(), possibleGuess};
            if (newRes.prob > res.prob) {
                res = newRes;
                if (res.prob == 1.00) {
                    return setCacheVal(key, res);
                }
            }
        }

        return setCacheVal(key, res);
    }

    BestWordResult getBestWord(const std::vector<IndexType> &answers, const std::vector<IndexType> &_guesses, uint8_t triesRemaining) {
        assertm(answers.size() != 0, "NO WORDS!");
        assertm(triesRemaining != 0, "no tries remaining");

        if (triesRemaining >= answers.size()) return BestWordResult {1.00, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {1.00/answers.size(), answers[0]};
        }

        std::vector<IndexType> clearedGuesses;
        const auto &guesses = isEasyMode
            ? (clearedGuesses = clearGuesses(_guesses, answers))
            : _guesses;

        auto wsAnswers = buildAnswersWordSet(answers);
        auto wsGuesses = buildGuessesWordSet(guesses);
        auto key = getCacheKey(wsAnswers, wsGuesses, triesRemaining);

        auto cacheVal = getFromCache(key);
        if (cacheVal.prob != -1) return cacheVal;

        BestWordResult res = {-1.00, -1};
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(reverseIndexLookup[possibleGuess] << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            auto prob = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetter(reverseIndexLookup[actualWordIndex]);
                auto state = AttemptStateToUse(getter);

                const auto answerWords = state.guessWord(possibleGuess, answers, reverseIndexLookup);
                BestWordResult pr;
                if constexpr (isEasyMode) {
                    pr = getBestWord(answerWords, guesses, triesRemaining-1);
                } else {
                    const auto nextGuessWords = state.guessWord(possibleGuess, guesses, reverseIndexLookup);
                    pr = getBestWord(answerWords, nextGuessWords, triesRemaining-1);
                }
                prob += pr.prob;
                if (EARLY_EXIT && pr.prob != 1) break;
                else if ((1.00-pr.prob) * answerWords.size() > maxIncorrect) break;
            }

            BestWordResult newRes = {prob/answers.size(), possibleGuess};
            if (newRes.prob > res.prob) {
                res = newRes;
                if (res.prob == 1.00) {
                    return setCacheVal(key, res);
                }
            }
        }

        return setCacheVal(key, res);
    }

    void precompute() {
        GUESSESSOLVER_DEBUG("precompute AnswersAndGuessesSolver.");
        //AttemptState::setupWordCache(allGuesses.size());
        getBestWordCache = {};
        buildClearGuessesInfo();
    }

    inline std::vector<IndexType> clearGuesses(std::vector<IndexType> guesses, const std::vector<IndexType> &answers) {   
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        //auto sizeBefore = guesses.size();
        std::erase_if(guesses, [&](const int &guessIndex) {
            return (answerLetterMask & letterCountLookup[guessIndex]) == 0;
        });
        //auto numRemoved = sizeBefore - guesses.size();
        //if (numRemoved > 0) DEBUG("REMOVED " << getPerc(numRemoved, sizeBefore));

        return guesses;
    }

    inline static std::vector<int> letterCountLookup = {};
    void buildClearGuessesInfo() {
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

    AnswersAndGuessesKey getCacheKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, uint8_t triesRemaining) {
        return AnswersAndGuessesKey(wsAnswers, wsGuesses, triesRemaining);
    }

    BestWordResult getFromCache(const AnswersAndGuessesKey &key) {
        auto it = getBestWordCache.find(key);
        if (it == getBestWordCache.end()) {
            cacheMiss++;
            return {-1, -1};
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

public:
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
};
