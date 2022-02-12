#pragma once
#include "AttemptState.h"

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
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) DEBUG(x)

template <bool isEasyMode>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(const std::vector<std::string> &allAnswers, const std::vector<std::string> &allGuesses, int maxTries = MAX_TRIES)
        : allAnswers(allAnswers),
          allGuesses(allGuesses),
          allAnswersSet(allAnswers.begin(), allAnswers.end()),
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

            std::unordered_set<std::string> allGuessesSet{allGuesses.begin(), allGuesses.end()};
            for (auto ans: allAnswers) {
                if (allGuessesSet.count(ans) == 0) {
                    DEBUG("possible answer is missing in guesses: " << ans);
                    exit(1);
                }
            }
        }
    
    using ReverseIndexType = std::unordered_map<std::string, int>;
    const std::vector<std::string> allAnswers, allGuesses;
    const std::unordered_set<std::string> allAnswersSet;
    int maxTries;

    std::string startingWord = "blahs";
    ReverseIndexType reverseIndexAnswers = {}, reverseIndexGuesses = {};
    std::unordered_map<AnswersAndGuessesKey, BestWordResult> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;
    const bool useExactSearch = true; // only if probablity is 1.00

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
        if (allAnswersSet.count(answer) == 0) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter);
        std::vector<std::string> answers = allAnswers, guesses = allGuesses;

        if (isEasyMode) {
            guesses = clearGuesses(guesses, answers);
        }

        std::string guess = startingWord;// firstPr.second;
        if (getFirstWord) {
            DEBUG("guessing first word...");
            auto firstPr = getBestWord(answers, guesses, maxTries);
            DEBUG("first word: " << firstPr.word << ", known prob: " << firstPr.prob);
            guess = firstPr.word;
        }

        for (int tries = 1; tries <= maxTries; ++tries) {
            if (guess == answer) return tries;
            if (tries == maxTries) break;
            answers = state.guessWord(guess, answers).words;
            if (!isEasyMode) guesses = state.guessWord(guess, guesses).words;

            GUESSESSOLVER_DEBUG(answer << ", " << tries << ": words size: " << answers.size() << ", guesses size: " << guesses.size());
            auto pr = getBestWord(answers, guesses, maxTries-tries);
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << pr.word << ", PROB: " << pr.prob);

            if (useExactSearch && pr.prob != 1.00) break;

            guess = pr.word;
        }
        return -1;
    }
    

    BestWordResult getBestWord(const std::vector<std::string> &answers, const std::vector<std::string> &guesses, uint8_t triesRemaining) {
        if (answers.size() == 0) { DEBUG("NO WORDS!"); exit(1); }
        if (triesRemaining == 0) return {1.00/answers.size(), answers[0]}; // no guesses left.
        if (triesRemaining >= answers.size()) return BestWordResult {1.00, answers[0]};

        if (triesRemaining == 1) { // we can't use info from last guess
            return {1.00/answers.size(), answers[0]};
        }

        /*const std::vector<std::string> guessWords = isEasyMode
            ? clearGuesses(guesses, answers)
            : guesses;*/

        auto wsAnswers = buildAnswersWordSet(answers);
        auto wsGuesses = buildGuessesWordSet(guesses);
        auto key = getCacheKey(wsAnswers, wsGuesses, triesRemaining);

        auto cacheVal = getFromCache(key);
        if (cacheVal.prob != -1) return cacheVal;

        BestWordResult res = {-1.00, ""};
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        for (std::size_t myInd = 0; myInd < guesses.size(); myInd++) {
            const auto &possibleGuess = guesses[myInd];
            if (triesRemaining == maxTries) DEBUG(possibleGuess << ": " << triesRemaining << ": " << getPerc(myInd, guesses.size()));
            auto prob = 0.00;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWord = answers[i];
                auto getter = PatternGetter(actualWord);
                auto state = AttemptState(getter);

                const auto answerWords = state.guessWordCached(possibleGuess, answers).words;
                const auto nextGuessWords = isEasyMode
                    ? guesses
                    : state.guessWordCached(possibleGuess, guesses).words;

                auto pr = getBestWord(answerWords, nextGuessWords, triesRemaining-1);
                prob += pr.prob;
                if (useExactSearch && pr.prob != 1) break;
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

    inline std::vector<std::string> clearGuesses(std::vector<std::string> guesses, const std::vector<std::string> &answers) {
        MinLetterType lettersInAnswers = {};
        for (auto &ans: answers) {
            for (auto c: ans) lettersInAnswers[c-'a'] = 1;
        }
        auto sizeBefore = guesses.size();
        std::erase_if(guesses, [&](const std::string &s) {
            return std::all_of(s.begin(), s.end(), [&](const char c) { return lettersInAnswers[c-'a'] == 0; });
        });
        //auto numRemoved = sizeBefore - guesses.size();
        //if (numRemoved > 0) DEBUG("REMOVED " << getPerc(numRemoved, sizeBefore));
        return guesses;
    }


    // hash table
    void prepareHashTable() {
        reverseIndexAnswers.clear();
        reverseIndexGuesses.clear();

        for (std::size_t i = 0; i < allAnswers.size(); ++i) {
            reverseIndexAnswers[allAnswers[i]] = i;
        }
        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            reverseIndexGuesses[allGuesses[i]] = i;
        }
        DEBUG("RESETTING ANSWERSANDGUESSESSOLVER CACHE");
        getBestWordCache = {};
    }

    AnswersAndGuessesKey getCacheKey(const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, int triesRemaining) {
        return AnswersAndGuessesKey(wsAnswers, wsGuesses, triesRemaining);
    }

    BestWordResult getFromCache(const AnswersAndGuessesKey &key) {
        auto it = getBestWordCache.find(key);
        if (it == getBestWordCache.end()) {
            cacheMiss++;
            return {-1, ""};
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
};
