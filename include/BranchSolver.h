#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"
#include "WordSetUtil.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>

using WordProbPair = std::pair<double, std::string>;

struct BranchSolver {
    BranchSolver(const std::vector<std::string> &allWords)
        : allWords(allWords)
        {}
    
    const std::vector<std::string> allWords;
    std::string startingWord = "";
    std::unordered_map<std::string, int> reverseIndex = {};
    std::array<std::unordered_map<WordSet, WordProbPair>, MAX_TRIES+1> getBestWordCache;
    long long cacheSize = 0, cacheMiss = 0, cacheHit = 0;

    void precompute() {
        DEBUG("getting starting word...");
        START_TIMER(startWord);
        prepareHashTable(allWords);
        startingWord = "aaron";//res.second;

        AttemptState::setupWordCache(allWords.size());

        //auto res = getBestWord(allWords, 0);
        //DEBUG("starting word " << res.second << " with PROB: " <<  res.first);
        //startingWord = res.second;
        END_TIMER(startWord);

        DEBUG("working out cache size...");
    }

    int solveWord(const std::string &answer) {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter, allWords);

        auto guess = startingWord;
        for (int i = 1; i <= MAX_TRIES; ++i) {
            if (guess == answer) { return i; }
            if (i == MAX_TRIES) break;
            auto newState = state.guessWord(guess);
            DEBUG(answer << ", " << i << ": words size: " << newState.words.size());
            auto pr = getBestWord(newState.words, newState.tries);
            DEBUG("NEXT GUESS: " << pr.second << ", PROB: " << pr.first);
            guess = pr.second;
            state = newState;
        }
        return -1;
    }

    std::vector<std::string> sortByProbability(const std::vector<std::string> &words) {
        int letterCount[26], totalLetters = 0;
        for (int i = 0; i < 26; ++i) letterCount[i] = 0;

        for (auto word: words) {
            for (auto c: word) {
                letterCount[c-'a']++;
                totalLetters++;
            }
        }

        std::vector<WordProbPair> newWords(words.size());
        for (std::size_t i = 0; i < words.size(); ++i) {
            double prob = 0.00;
            for (auto c: words[i]) prob += letterCount[c-'a'];
            newWords[i] = {prob, words[i]};
        }
        std::sort(newWords.begin(), newWords.end(), std::greater<WordProbPair>());
        std::vector<std::string> ret(words.size());
        for (std::size_t i = 0; i < words.size(); ++i) {
            ret[i] = newWords[i].second;
        }
        return ret;
    }

    WordProbPair getBestWord(const std::vector<std::string> &words, int tries) {
        if (words.size() == 1) return {1.00, words[0]};

        // we can't use the information of the last guess...
        if (tries >= MAX_TRIES-1) return {1.00 / words.size(), words[0]};

        auto ws = buildWordSet(words);

        auto cacheVal = getFromCache(ws, tries);
        if (cacheVal.first != -1) return cacheVal;

        WordProbPair res = {-1.00, ""};
        //auto myWords = sortByProbability(words);

        for (auto possibleGuess: words) {
            auto prob = 0.00;
            for (auto actualWord: words) {
                auto getter = PatternGetter(actualWord);
                auto state = AttemptState(getter, tries, words);
                auto newState = state.guessWord(possibleGuess);
                //auto newState = state.guessWordCached(possibleGuess, reverseIndex[actualWord], reverseIndex[possibleGuess], ws, state.tries);

                //auto newWordSet = buildWordSet(ws, newState.deletedWords);
                auto pr = getBestWord(newState.words, newState.tries);
                prob += pr.first;
            }

            WordProbPair newPair = {prob/words.size(), possibleGuess};
            if (newPair.first > res.first) {
                res = newPair;
                if (res.first == 1.00) {
                    return setCacheVal(ws, tries, res);
                }
            }
        }

        return setCacheVal(ws, tries, res);
    }

    #pragma region hash table

    void prepareHashTable(const std::vector<std::string> &words) {
        reverseIndex.clear();
        for (std::size_t i = 0; i < words.size(); ++i) {
            reverseIndex[words[i]] = i;
        }
        for (auto i = 0; i <= MAX_TRIES; ++i) getBestWordCache[i] = {};
    }

    inline WordProbPair getFromCache(const std::vector<std::string> &words, int tries) {
        return getFromCache(buildWordSet(words), tries);
    }

    WordProbPair getFromCache(const WordSet &ws, int tries) {
        auto it = getBestWordCache[tries].find(ws);
        if (it == getBestWordCache[tries].end()) {
            cacheMiss++;
            return {-1, ""};
        }
        cacheHit++;
        return it->second;
    }

    inline WordProbPair setCacheVal(const std::vector<std::string> &words, int tries, const WordProbPair &res) {
        auto ws = buildWordSet(words);
        return setCacheVal(ws, tries, res);
    }

    inline WordProbPair setCacheVal(const WordSet &ws, int tries, const WordProbPair &res) {
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
};
