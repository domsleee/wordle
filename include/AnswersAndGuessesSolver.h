#pragma once
#include "AttemptState.h"
#include "AttemptStateKey.h"

#include "PatternGetter.h"
#include "WordSetUtil.h"
#include "BestWordResult.h"
#include "BranchDefs.h"
#include "AnswersAndGuessesKey.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>

#define GUESSESSOLVER_DEBUG(x) DEBUG(x)


struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(const std::vector<std::string> &allAnswers, const std::vector<std::string> &allGuesses, int maxTries = MAX_TRIES)
        : allAnswers(allAnswers),
          allGuesses(allGuesses),
          maxTries(maxTries)
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
        }
    
    using ReverseIndexType = std::unordered_map<std::string, int>;
    const std::vector<std::string> allAnswers, allGuesses;
    int maxTries;

    std::string startingWord = "blahs";
    ReverseIndexType reverseIndexAnswers = {}, reverseIndexGuesses = {};
    std::unordered_map<AnswersAndGuessesKey, BestWordResult> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;

    void precompute() {
        GUESSESSOLVER_DEBUG("precompute AnswersAndGuessesSolver.");
        //AttemptState::setupWordCache(allGuesses.size());
        prepareHashTable();
    }

    void setStartWord(const std::string &word) {
        DEBUG("set starting word "<< word);
        startingWord = word;
    }

    int solveWord(const std::string &answer, bool getFirstWord = false) {
        auto getter = PatternGetter(answer);
        auto answersState = AttemptState(getter, allAnswers);
        auto guessesState = AttemptState(getter, allGuesses);

        std::string guess = startingWord;// firstPr.second;
        if (getFirstWord) {
            auto firstPr = getBestWord(answersState.globLetterMinLimit, answersState.words, guessesState.words, 0);
            //DEBUG("known prob: " << firstPr.prob);
            guess = firstPr.word;
        }

        for (int i = 1; i <= maxTries; ++i) {
            if (guess == answer) return i;
            if (i == maxTries) break;
            answersState = answersState.guessWord(guess);
            guessesState = guessesState.guessWord(guess);

            auto answers = answersState.words;
            GUESSESSOLVER_DEBUG(answer << ", " << i << ": words size: " << answers.size());
            auto pr = getBestWord(answersState.globLetterMinLimit, answers, guessesState.words, answersState.tries, 2);
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << pr.word << ", PROB: " << pr.prob);
            //if (pr.prob != 1.00) break;

            guess = pr.word;
        }
        return -1;
    }
    

    BestWordResult getBestWord(const MinLetterType &minLetterLimit, const std::vector<std::string> &answers, const std::vector<std::string> &guesses, int tries, int triesRemaining=MAX_TRIES) {
        triesRemaining = std::min(triesRemaining, maxTries-tries);

        if (answers.size() == 0) { DEBUG("NO WORDS!"); exit(1); }
        if (triesRemaining == 0) return {1.00/answers.size(), answers[0], MAX_LOWER_BOUND}; // no guesses left.
        if (triesRemaining >= answers.size()) return BestWordResult {1.00, answers[0], tries+1};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {1.00/answers.size(), answers[0], MAX_LOWER_BOUND};
        }

        auto wsAnswers = buildAnswersWordSet(answers);
        auto wsGuesses = buildGuessesWordSet(guesses);
        auto key = getCacheKey(minLetterLimit, wsAnswers, wsGuesses, tries, triesRemaining);

        auto cacheVal = getFromCache(key);
        if (cacheVal.prob != -1) return cacheVal;

        BestWordResult res = {-1.00, "", MAX_LOWER_BOUND};
        //DEBUG("TRIES: " << tries);
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (auto newTriesRemaining = triesRemaining; newTriesRemaining <= triesRemaining; ++newTriesRemaining) {
            for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
                auto possibleGuess = guesses[myInd];
                //if (tries==1) DEBUG(possibleGuess << ": " << getPerc(myInd, guesses.size()));
                //if (tries==1 && myInd > 2) break;
                auto prob = 0.00;
                for (std::size_t i = 0; i < answers.size(); ++i) {
                    auto actualWord = answers[i];
                    auto getter = PatternGetter(actualWord);
                    auto answersState = AttemptState(getter, tries, answers, minLetterLimit);
                    auto guessesState = AttemptState(getter, tries, guesses, minLetterLimit);

                    //auto guessesKey = AttemptStateKey(minLetterLimit, wsGuesses, reverseIndexGuesses[possibleGuess], reverseIndexAnswers[actualWord]);
                    //auto answersKey = AttemptStateKey(minLetterLimit, wsAnswers, reverseIndexGuesses[possibleGuess], reverseIndexAnswers[actualWord]);

                    answersState = answersState.guessWord(possibleGuess); // answersState.guessWordFromAnswers(possibleGuess, answersKey, tries);
                    guessesState = guessesState.guessWord(possibleGuess); //guessesState.guessWord(possibleGuess);

                    auto pr = getBestWord(answersState.globLetterMinLimit, answersState.words, guessesState.words, tries+1, newTriesRemaining-1);
                    prob += pr.prob;
                    //if (pr.prob != 1) break; // only correct if solution is guaranteed
                }

                BestWordResult newRes = {prob/answers.size(), possibleGuess, MAX_LOWER_BOUND};
                if (newRes.prob > res.prob) {
                    res = newRes;
                    if (res.prob == 1.00) {
                        res.lowerBound = tries + newTriesRemaining;
                        return setCacheVal(key, res);
                    }
                }
            }
        }
        

        return setCacheVal(key, res);
    }

    #pragma region hash table

    void prepareHashTable() {
        reverseIndexAnswers.clear();
        reverseIndexGuesses.clear();

        for (std::size_t i = 0; i < allAnswers.size(); ++i) {
            reverseIndexAnswers[allAnswers[i]] = i;
        }
        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            reverseIndexGuesses[allGuesses[i]] = i;
        }
        DEBUG("RESETTING CACHE??");
        getBestWordCache = {};
    }

    AnswersAndGuessesKey getCacheKey(const MinLetterType &minLetterLimit, const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, int tries, int triesRemaining) {
        // tries is only relevant for upper bound.
        return AnswersAndGuessesKey(minLetterLimit, wsAnswers, wsGuesses, 0, triesRemaining);
    }

    BestWordResult getFromCache(const AnswersAndGuessesKey &key) {
        auto it = getBestWordCache.find(key);
        if (it == getBestWordCache.end()) {
            cacheMiss++;
            return {-1, "", MAX_LOWER_BOUND};
        }
        cacheHit++;
        return it->second;
    }

    inline BestWordResult setCacheVal(const AnswersAndGuessesKey &key, const BestWordResult &res) {
        if (getBestWordCache.count(key) == 0) {
            cacheSize++;
            getBestWordCache[key] = res;
        }
        return getBestWordCache[key];
    }

    template<typename T>
    T buildWordSet(const std::vector<std::string> &words, ReverseIndexType &reverseIndex) {
        auto wordset = T();
        for (auto word: words) {
            wordset.set(reverseIndex[word]);
        }
        return wordset;
    }

    WordSetAnswers buildAnswersWordSet(const std::vector<std::string> &answers) {
        return buildWordSet<WordSetAnswers>(answers, reverseIndexAnswers);
    }

    WordSetGuesses buildGuessesWordSet(const std::vector<std::string> &guesses) {
        return buildWordSet<WordSetGuesses>(guesses, reverseIndexGuesses);
    }

    #pragma endregion
};
