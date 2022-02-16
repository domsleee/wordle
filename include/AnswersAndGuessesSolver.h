#pragma once
#include "AttemptState.h"
#include "AttemptStateFast.h"

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

#define GUESSESSOLVER_DEBUG(x)

struct AnswersGuessesIndexesPair {
    std::vector<IndexType> answers = {};
    std::vector<IndexType> guesses = {};
    AnswersGuessesIndexesPair(std::size_t numAnswers, std::size_t numGuesses) {
        answers = getVector(numAnswers, 0);
        guesses = getVector(numGuesses, numAnswers);
    }
};

template <bool isEasyMode>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(const std::vector<std::string> &allAnswers, const std::vector<std::string> &allGuesses, int maxTries = MAX_TRIES)
        : allAnswers(allAnswers),
          allGuesses(allGuesses),
          reverseIndexLookup(buildLookup()),
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
    
    const std::vector<std::string> allAnswers, allGuesses;
    const std::vector<std::string> reverseIndexLookup;
    const int maxTries;

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

        int firstWordIndex = std::distance(
            reverseIndexLookup.begin(),
            std::find(reverseIndexLookup.begin() + allAnswers.size(), reverseIndexLookup.end(), startingWord)
        );

        auto p = AnswersGuessesIndexesPair(allAnswers.size(), allGuesses.size());
        if (getFirstWord) {
            DEBUG("guessing first word...");
            auto firstPr = getBestWord(p.answers, p.guesses, maxTries);
            DEBUG("first word: " << reverseIndexLookup[firstPr.wordIndex] << ", known prob: " << firstPr.prob);
            firstWordIndex = firstPr.wordIndex;
            if (useExactSearch && firstPr.prob != 1.00) return -1;
        }

        return solveWord(answer, firstWordIndex, p);
    }

    int solveWord(const std::string &answer, int firstWordIndex, AnswersGuessesIndexesPair &p) {
        auto getter = PatternGetter(answer);
        auto state = AttemptStateFast(getter);
        
        int guessIndex = firstWordIndex;
        for (int tries = 1; tries <= maxTries; ++tries) {
            if (reverseIndexLookup[guessIndex] == answer) return tries;
            if (tries == maxTries) break;

            makeGuess(p, state, guessIndex, reverseIndexLookup);

            GUESSESSOLVER_DEBUG(answer << ", " << tries << ": words size: " << answerIndexes.size() << ", guesses size: " << guessIndexes.size());
            auto pr = getBestWord(p.answers, p.guesses, maxTries-tries);
            GUESSESSOLVER_DEBUG("NEXT GUESS: " << reverseIndexLookup[pr.wordIndex] << ", PROB: " << pr.prob);

            if (EARLY_EXIT && pr.prob != 1.00) break;

            guessIndex = pr.wordIndex;
        }
        return -1;
    }


    void makeGuess(AnswersGuessesIndexesPair &pair, const AttemptStateFast &state, IndexType guessIndex, const std::vector<std::string> &reverseIndexLookup) {
        pair.answers = state.guessWord(guessIndex, pair.answers, reverseIndexLookup);
        if (!isEasyMode) pair.guesses = state.guessWord(guessIndex, pair.guesses, reverseIndexLookup);
    }

private:

    
    BestWordResult getBestWord(const std::vector<IndexType> &answers, const std::vector<IndexType> &_guesses, uint8_t triesRemaining) {
        if (answers.size() == 0) {
            DEBUG("NO WORDS!"); exit(1);
        }
        if (triesRemaining == 0) return {1.00/answers.size(), answers[0]}; // no guesses left.
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
            int correct = 0;
            for (std::size_t i = 0; i < answers.size(); ++i) {
                const auto &actualWordIndex = answers[i];
                auto getter = PatternGetter(reverseIndexLookup[actualWordIndex]);
                auto state = AttemptStateFast(getter);

                const auto answerWords = state.guessWord(possibleGuess, answers, reverseIndexLookup);
                BestWordResult pr;
                if (isEasyMode) {
                    pr = getBestWord(answerWords, guesses, triesRemaining-1);
                } else {
                    const auto nextGuessWords = state.guessWord(possibleGuess, guesses, reverseIndexLookup);
                    pr = getBestWord(answerWords, nextGuessWords, triesRemaining-1);
                }
                prob += pr.prob;
                if (EARLY_EXIT && pr.prob != 1) break;
                if (pr.prob == 1.00) correct++;
                else if ((1.00-pr.prob) * answerWords.size() > MAX_INCORRECT) break;
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
        std::vector<std::string> lookup(allAnswers.size() + allGuesses.size());

        for (std::size_t i = 0; i < allAnswers.size(); ++i) {
            lookup[i] = allAnswers[i];
        }
        for (std::size_t i = 0; i < allGuesses.size(); ++i) {
            lookup[i + allAnswers.size()] = allGuesses[i];
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
            //if (key.triesRemaining == MAX_TRIES) return res;
            getBestWordCache[key] = res;
        }
        return getBestWordCache[key];
    }

    template<typename T>
    T buildWordSet(const std::vector<IndexType> &wordIndexes, int offset) {
        auto wordset = T();
        for (auto wordIndex: wordIndexes) {
            wordset.set(offset + wordIndex);
        }
        return wordset;
    }

    WordSetAnswers buildAnswersWordSet(const std::vector<IndexType> &answers) {
        return buildWordSet<WordSetAnswers>(answers, 0);
    }

    WordSetGuesses buildGuessesWordSet(const std::vector<IndexType> &guesses) {
        return buildWordSet<WordSetGuesses>(guesses, -allAnswers.size());
    }
};
