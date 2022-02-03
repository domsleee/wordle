#pragma once
#include "AttemptState.h"
#include "PatternGetter.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>


using WordProbPair = std::pair<double, std::string>;
using WordSet = std::bitset<350>;

struct BranchSolver {
    BranchSolver(const std::vector<std::string> &allWords)
        : allWords(allWords)
        {}
    
    const std::vector<std::string> allWords;
    std::string startingWord = "";
    std::unordered_map<std::string, int> reverseIndex = {};
    std::array<std::unordered_map<WordSet, WordProbPair>, 7> getBestWordCache;

    void precompute() {
        DEBUG("getting starting word...");
        START_TIMER(startWord);
        prepareHashTable(allWords);
        //auto res = getBestWord(allWords, 0);
        //DEBUG("starting word " << res.second << " with PROB: " <<  res.first);
        startingWord = "aaron";//res.second;
        END_TIMER(startWord);
    }

    int solveWord(const std::string &answer) {
        auto getter = PatternGetter(answer);
        auto state = AttemptState(getter, allWords);

        prepareHashTable(allWords);

        auto guess = startingWord;
        for (int i = 1; i <= 6; ++i) {
            if (guess == answer) { return i; }
            if (i == 6) break;
            auto newState = state.guessWord(guess);
            DEBUG(answer << ", " << i << ": words size: " << newState.words.size());
            auto pr = getBestWord(newState.words, newState.tries);
            DEBUG("NEXT GUESS: " << pr.second << ", PROB: " << pr.first);
            guess = pr.second;
            state = newState;
        }
        return -1;
    }

    WordProbPair getBestWord(const std::vector<std::string> &words, int tries) {
        if (words.size() == 1) return {1.00, words[0]};

        // we can't use the information of the last guess...
        if (tries == 5) return {1.00 / words.size(), words[0]};

        auto cacheVal = getFromCache(words, tries);
        if (cacheVal.first != -1) return cacheVal;

        WordProbPair res = {-1.00, ""};
        for (auto possibleGuess: words) {
            auto prob = 0.00;
            for (auto actualWord: words) {
                auto getter = PatternGetter(actualWord);
                auto state = AttemptState(getter, tries, words);
                auto newState = state.guessWord(possibleGuess);
                auto pr = getBestWord(newState.words, newState.tries);
                prob += pr.first;
            }

            WordProbPair newPair = {prob/words.size(), possibleGuess};
            if (newPair.first > res.first) {
                res = newPair;
            }
        }

        return setCacheVal(words, tries, res);
    }

    #pragma region hash table

    void prepareHashTable(const std::vector<std::string> &words) {
        reverseIndex.clear();
        for (std::size_t i = 0; i < words.size(); ++i) {
            reverseIndex[words[i]] = i;
        }
        for (auto i = 0; i <= 6; ++i) getBestWordCache[i] = {};
    }

    inline WordProbPair getFromCache(const std::vector<std::string> &words, int tries) {
        auto it = getBestWordCache[tries].find(buildWordSet(words));
        if (it == getBestWordCache[tries].end()) {
            return {-1, ""};
        }
        return it->second;
    }

    inline WordProbPair setCacheVal(const std::vector<std::string> &words, int tries, const WordProbPair &res) {
        return getBestWordCache[tries][buildWordSet(words)] = res;
    }

    WordSet buildWordSet(const std::vector<std::string> &words) {
        auto wordset = WordSet();
        for (auto word: words) {
            wordset.set(reverseIndex[word]);
        }
        return wordset;

        /*
        std::vector<int> wordSet(words.size());
        for (std::size_t i = 0; i < words.size(); ++i) {
            wordSet[i] = reverseIndex[words[i]];
        }
        return wordSet;*/
    }

    #pragma endregion
};
