#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"
#include "BranchDefs.h"
#include "BestWordResult.h"

#include "WordSetUtil.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>

#define BRANCHSOLVER_DEBUG(x)

struct BranchSolver {
    BranchSolver(const std::vector<std::string> &allWords, int maxTries = MAX_TRIES, bool automaticallySetup4Words = false)
        : allWords(allWords),
          maxTries(maxTries),
          automaticallySetup4Words(automaticallySetup4Words)
        {
            resetWsForTry();
        }
    
    void resetWsForTry() {
        for (auto &ws: wsForTry) {
            for (auto i = 0; i < ws.size(); ++i) ws.set(i);
        }
    }
    
    const std::vector<std::string> allWords;
    std::string startingWord = "";
    std::unordered_map<std::string, int> reverseIndex = {};
    std::array<std::unordered_map<WordSet, BestWordResult>, MAX_TRIES+1> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;
    int maxTries;
    using WsForTryType = std::array<WordSet, MAX_TRIES+1>;
    WsForTryType wsForTry;
    bool automaticallySetup4Words;
    bool shouldReduceFromTrie = false;

    void precompute() {
        BRANCHSOLVER_DEBUG("getting starting word...");
        //START_TIMER(startWord);
        prepareHashTable(allWords);
        startingWord = "soare";//res.second;

        AttemptState::setupWordCache(allWords.size());

        //auto res = getBestWord(allWords, 0);
        //DEBUG("starting word " << res.second << " with PROB: " <<  res.first);
        //startingWord = res.second;
        //END_TIMER(startWord);
        
        //if (automaticallySetup4Words) setupInitial4Words();
    }

    void setupInitial4Words() {
        int lim = 4;
        //DEBUG("solve for all words with " << lim << " tries");
        START_TIMER(FOUR);
        resetWsForTry();
        int changeNo = 0;

        std::vector<std::vector<std::string>> wordSolutions(MAX_TRIES);
        for (int i = lim; i <= lim; ++i) {
            auto localWsForTry = wsForTry;
            wordSolutions[i] = precomputeForWords(allWords, i, localWsForTry);
        }
        DEBUG("first pass, found: " << getPerc(wordSolutions[lim].size(), allWords.size()));

        for (int i = lim; i <= lim; ++i) {
            auto checkedWordSolutions = precomputeForWords(wordSolutions[lim], maxTries, wsForTry);
            DEBUG("second pass (" << i << "), found: " << getPerc(checkedWordSolutions.size(), allWords.size()));
        }
    
        shouldReduceFromTrie = true;
        END_TIMER(FOUR);
        //exit(1);
    }

    std::vector<std::string> precomputeForWords(const std::vector<std::string> &wordsToCheck, int triesLim, WsForTryType &localWsForTry) {
        std::vector<std::string> wordSolutions;
        long long numChanges = 0;
        
        while (true) {
            DEBUG("SETTING UP INTIIAL " << triesLim << " " << getPerc(wordSolutions.size(), wordsToCheck.size()) << ", #changes: " << numChanges);
            numChanges = 0;

            auto bs = BranchSolver(allWords, triesLim);
            bs.precompute();
            bs.wsForTry = localWsForTry;
            bs.shouldReduceFromTrie = true;

            auto broken = false;
            wordSolutions = {};

            for (std::size_t i = 0; i < wordsToCheck.size(); ++i) {
                //DEBUG("perc: " << getPerc(i, wordsToCheck.size()));
                auto r = bs.solveWordWithLowerBound(wordsToCheck[i]);

                auto hasLowerBound = r.second != MAX_LOWER_BOUND;
                if (wordsToCheck[i] == "tiara") DEBUG(wordsToCheck[i] << ", " << r.first << ", lb: " << r.second);
                if (hasLowerBound) {
                    wordSolutions.push_back(wordsToCheck[i]);
                }

                auto localChange = false;
                auto ind = reverseIndex[wordsToCheck[i]];
                auto startInd = hasLowerBound ? r.second : 0;
                for (auto j = startInd; j <= maxTries; ++j) {
                    auto &ws1 = localWsForTry[j];
                    if (ws1.test(ind) && hasLowerBound) {
                        ws1.reset(ind);
                        localChange = true;
                    }
                    else if (!ws1.test(ind) && !hasLowerBound) {
                        DEBUG("REVERT");exit(1);
                        ws1.set(ind);
                        localChange = true;
                    }
                }
                broken |= localChange;
                if (localChange) numChanges++;
                if (broken) { /*break;*/ }
                
            }

            if (!broken) break;
        }

        return wordSolutions;
    }

    void setStartWord(const std::string &word) {
        DEBUG("set starting word "<< word);
        startingWord = word;
        setupInitial4Words();
    }

    int solveWord(const std::string &answer, bool getFirstWord = false) {
        return solveWordWithLowerBound(answer, getFirstWord).first;
    }

    std::pair<int, int> solveWordWithLowerBound(const std::string &answer, bool getFirstWord = false) {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter, allWords);
        int lowerBound = MAX_LOWER_BOUND;

        std::string guess = startingWord;// firstPr.second;
        if (getFirstWord) {
            auto firstPr = getBestWord(state.words, state.tries);
            //DEBUG("known prob: " << firstPr.prob);
            guess = firstPr.word;
        }
        if (guess == answer) return {1, 1};

        for (int i = 1; i <= maxTries; ++i) {
            if (guess == answer) { return {i, lowerBound}; }
            if (i == maxTries) break;
            auto newState = state.guessWord(guess);
            auto words = newState.words;
            if (words.size() == 1) {
                guess = words[0];
                continue;
            }
            auto reducedWords = reduceWordsFromTry(words, i);
            if (reducedWords.size() == 1) {
                DEBUG("JUICED");
                guess = reducedWords[0];
                if (guess != answer) {
                    DEBUG(i << ": REDUCED TO WRONG WORD? answer:" << answer << ", guess:" << guess);
                
                    auto wasInLast = std::find(newState.words.begin(), newState.words.end(), answer) != newState.words.end();
                    auto wasInBefore = std::find(state.words.begin(), state.words.end(), answer) != state.words.end();

                    DEBUG("was in last " << wasInLast << ", was in before: " << wasInBefore);
                    exit(1);
                }
                continue;
            }

            if (std::find(words.begin(), words.end(), answer) == words.end()) {
                DEBUG("MISSING WORD??" << answer << " after " << i);
                auto wasInLast = std::find(newState.words.begin(), newState.words.end(), answer) != newState.words.end();
                auto wasInBefore = std::find(state.words.begin(), state.words.end(), answer) != state.words.end();

                DEBUG("was in last " << wasInLast << ", was in before: " << wasInBefore);
                //exit(1);
            }
            BRANCHSOLVER_DEBUG(answer << ", " << i << ": words size: " << words.size());
            auto pr = getBestWord(words, newState.tries);
            BRANCHSOLVER_DEBUG("NEXT GUESS: " << pr.word << ", PROB: " << pr.prob);
            if (pr.prob != 1.00) return {-1, MAX_LOWER_BOUND};
            if (answer == "tiara") DEBUG("NEXT GUESS: " << pr.word << ", PROB: " << pr.prob);

            guess = pr.word;
            state = newState;
            if (pr.lowerBound > lowerBound) {
                DEBUG("ERROR: lower bound increased?? from: " << lowerBound << " -> " << pr.lowerBound);
                exit(1);
            }
            lowerBound = pr.lowerBound;
        }
        return {-1, MAX_LOWER_BOUND};
    }

    

    BestWordResult getBestWord(const std::vector<std::string> &inpWords, int tries, int triesRemaining=MAX_DEPTH) {
        triesRemaining = std::min(triesRemaining, maxTries-tries); // we know tries < maxTries-1
        if (inpWords.size() == 0) { DEBUG("NO WORDS!"); exit(1); }

        if (triesRemaining == 0) return {0.00, inpWords[0], MAX_LOWER_BOUND}; // no guesses left.

        if (inpWords.size() == 1) return BestWordResult {1.00, inpWords[0], tries+1};

        auto words = shouldReduceFromTrie ? reduceWordsFromTry(inpWords, tries) : inpWords;
        auto ws = buildWordSet(words);

        if (words.size() == 1) return {1.00, words[0], tries+1};
        if (words.size() == 0) {
            DEBUG("eliminated all words..."); exit(1);
        }

        if (triesRemaining == 1) { // we can't use info from last guess
            return {0.00, words[0], MAX_LOWER_BOUND};
        }
    
        auto cacheVal = getFromCache(ws, triesRemaining);
        if (cacheVal.prob != -1) return cacheVal;

        BestWordResult res = {-1.00, "", MAX_LOWER_BOUND};
        
        for (auto newTriesRemaining = 2; newTriesRemaining <= triesRemaining; ++newTriesRemaining) {
            for (auto possibleGuess: words) {
                //if (tries==0) DEBUG(possibleGuess << ": " << getPerc(myInd, words.size()));
                auto prob = 0.00;
                for (std::size_t i = 0; i < words.size(); ++i) {
                    auto actualWord = words[i];
                    auto getter = PatternGetter(actualWord);
                    auto state = AttemptState(getter, tries, words);
                    auto newState = state.guessWord(possibleGuess);
                    //auto newState = state.guessWordCached(possibleGuess, reverseIndex[actualWord], reverseIndex[possibleGuess], ws, state.tries);

                    auto pr = getBestWord(newState.words, newState.tries, newTriesRemaining-1);
                    prob += pr.prob;
                    if (pr.prob != 1) break; // only correct if solution is guaranteed
                }

                BestWordResult newRes = {prob/words.size(), possibleGuess, MAX_LOWER_BOUND};
                if (newRes.prob > res.prob) {
                    res = newRes;
                    if (res.prob == 1.00) {
                        res.lowerBound = tries + newTriesRemaining;
                        return setCacheVal(ws, triesRemaining, res);
                    }
                }
            }
        }
        

        return setCacheVal(ws, triesRemaining, res);
    }

    std::vector<std::string> reduceWordsFromTry(const std::vector<std::string> &words, int tries) {
        auto ws = buildWordSet(words);
        ws &= wsForTry[tries];
        return buildWordsFromWordSet(ws);
    }

    #pragma region hash table

    void prepareHashTable(const std::vector<std::string> &words) {
        reverseIndex.clear();
        for (std::size_t i = 0; i < words.size(); ++i) {
            reverseIndex[words[i]] = i;
        }
        for (auto i = 0; i <= maxTries; ++i) getBestWordCache[i] = {};
    }

    inline BestWordResult getFromCache(const std::vector<std::string> &words, int tries) {
        return getFromCache(buildWordSet(words), tries);
    }

    BestWordResult getFromCache(const WordSet &ws, int tries) {
        auto it = getBestWordCache[tries].find(ws);
        if (it == getBestWordCache[tries].end()) {
            cacheMiss++;
            return {-1, "", 100};
        }
        cacheHit++;
        return it->second;
    }

    inline BestWordResult setCacheVal(const std::vector<std::string> &words, int tries, const BestWordResult &res) {
        auto ws = buildWordSet(words);
        return setCacheVal(ws, tries, res);
    }

    inline BestWordResult setCacheVal(const WordSet &ws, int tries, const BestWordResult &res) {
        if (getBestWordCache[tries].count(ws) == 0) {
            cacheSize++;
            getBestWordCache[tries][ws] = res;
        }
        return getBestWordCache[tries][ws];
    }

    WordSet buildWordSet(const std::vector<std::string> &words) {
        auto wordset = WordSet();
        for (auto word: words) {
            wordset.set(reverseIndex[word]);
        }
        return wordset;
    }

    WordSet buildWordSet(const WordSet &ws, const std::vector<std::string> &deletedWords) {
        auto wordset = ws;
        for (auto word: deletedWords) {
            wordset.reset(reverseIndex[word]);
        }
        return wordset;
    }

    #pragma endregion

    std::vector<std::string> buildWordsFromWordSet(const WordSet &ws) {
        std::vector<std::string> res;
        for (auto i = 0; i < ws.size(); ++i) {
            if (ws.test(i)) res.push_back(allWords[i]);
        }
        return res;
    }
};
