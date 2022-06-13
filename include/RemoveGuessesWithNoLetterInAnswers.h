#pragma once
#include "GlobalState.h"
#include "PerfStats.h"
#include "NonLetterLookup.h"
#include "RemoveGuessesPartitions.h"
#include "RemoveGuessesWithBetterGuessCache.h"

using namespace NonLetterLookupHelpers;

struct RemoveGuessesWithNoLetterInAnswers {
    inline static std::vector<int> letterCountLookup = {};

    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &guesses, const int nonLetterMask) {
        //auto numNonLetters = __builtin_popcount(nonLetterMask);

        std::vector<int> removedIndexes(GlobalState.allGuesses.size(), 0);
        std::erase_if(guesses, [&](auto guessIndex) {
            // if (answersCheck && answersWs[guessIndex]) return false;
            for (auto otherGuessIndex: guesses) {
                if (otherGuessIndex == guessIndex || removedIndexes[otherGuessIndex]) continue;
                bool ok = true;
                for (int i = 0; i < 5; ++i) {
                    char c = GlobalState.reverseIndexLookup[guessIndex][i];
                    bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
                    if (knownNonLetter) continue;
                    if (c != GlobalState.reverseIndexLookup[otherGuessIndex][i]) {
                        ok = false;
                        break;
                    }
                }
                if (ok) {
                    removedIndexes[guessIndex] = true;
                    return true;
                }
            }
            return false;
        });
    }

    static void removeWithBetterOrSameGuessFast(PerfStats &stats, GuessesVec &guesses, const int nonLetterMask) {
        std::vector<int8_t> guessToNumNonLetters(GlobalState.allGuesses.size(), 0);
        for (auto guessIndex: guesses) {
            int8_t r = 0;
            for (char c: GlobalState.reverseIndexLookup[guessIndex]) {
                bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
                r += knownNonLetter == true;
            }
            bool isPossibleAnswer = guessIndex < GlobalState.allAnswers.size();
            guessToNumNonLetters[guessIndex] = 2*r + isPossibleAnswer;
        }
        std::sort(guesses.begin(), guesses.end(), [&](auto a, auto b) { return guessToNumNonLetters[a] < guessToNumNonLetters[b];});

        std::unordered_map<std::string, IndexType> stringToFirstSeen = {};
        std::vector<std::string> indexToRepString(GlobalState.allGuesses.size());
        for (auto guessIndex: guesses) {
            auto repString = GlobalState.reverseIndexLookup[guessIndex];
            for (int i = 0; i < 5; ++i) {
                char c = GlobalState.reverseIndexLookup[guessIndex][i];
                bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
                if (knownNonLetter) {
                    repString[i] = '.';
                }
            }
            indexToRepString[guessIndex] = repString;
            auto res = stringToFirstSeen.try_emplace(repString, guessIndex);
            if (!res.second) continue;

            std::function<void(std::string, int)> removeSubs = [&](std::string s, int toMutate) {
                if (toMutate == 5) return;

                removeSubs(s, toMutate+1);
                if (s[toMutate] != '.') {
                    s[toMutate] = '.';
                    stringToFirstSeen.emplace(s, guessIndex); //steamroll
                    removeSubs(s, toMutate+1);
                }
            };
            stats.tick(44);
            removeSubs(repString, 0);
            stats.tock(44);
        }

        std::erase_if(guesses, [&](auto guessIndex) {
            return stringToFirstSeen[indexToRepString[guessIndex]] != guessIndex;
        });

        // computeGuessToNumGroups();
        //std::sort(guesses.begin(), guesses.end(), [&](auto a, auto b) { return guessToNumGroups[a] > guessToNumGroups[b];});        
    }

    static void removeWithBetterOrSameGuessFaster(PerfStats &stats, GuessesVec &guesses, const int nonLetterMask) {
        stats.tick(11);
        std::vector<int8_t> guessToNumNonLetters(GlobalState.allGuesses.size(), 0);
        for (auto guessIndex: guesses) {
            guessToNumNonLetters[guessIndex] = getNumNonLetters(guessIndex, nonLetterMask);
        }
        std::stable_sort(guesses.begin(), guesses.end(), [&](auto a, auto b) { return guessToNumNonLetters[a] < guessToNumNonLetters[b];});

        static thread_local std::vector<int> guessIndexToNodeId;
        static thread_local std::vector<IndexType> nodeIdToFirstSeen;
        guessIndexToNodeId.resize(GlobalState.allGuesses.size());
        nodeIdToFirstSeen.assign(NonLetterLookup::nodes.size(), MAX_INDEX_TYPE);
        std::stack<int> nodeIdsToMark = {};
        stats.tock(11);

        stats.tick(12);
        for (auto guessIndex: guesses) {
            auto nodeId = getNodeId(guessIndex, nonLetterMask);
            guessIndexToNodeId[guessIndex] = nodeId;

            if (nodeIdToFirstSeen[nodeId] != MAX_INDEX_TYPE) continue;
            nodeIdToFirstSeen[nodeId] = guessIndex;

            nodeIdsToMark.push(nodeId);
            while (!nodeIdsToMark.empty()) {
                auto t = nodeIdsToMark.top(); nodeIdsToMark.pop();
                //DEBUG("child: " << NonLetterLookup::idToStringPattern[t] << ": (t: " << t << ") #childreN: " << NonLetterLookup::nodes[t].childIds.size());
                // doesn't make sense to use compressedNodes here.
                // example: .ss.. will not be marked by .ss.s
                for (auto childId: NonLetterLookup::nodes[t].childIds) {
                    if (nodeIdToFirstSeen[childId] != MAX_INDEX_TYPE) continue;
                    nodeIdToFirstSeen[childId] = guessIndex;
                    nodeIdsToMark.push(childId);
                }
            }
        }
        stats.tock(12);

        stats.tick(13);
        std::erase_if(guesses, [&](auto guessIndex) {
            return nodeIdToFirstSeen[guessIndexToNodeId[guessIndex]] != guessIndex;
        });
        stats.tock(13);
    }


    static int8_t getNumNonLetters(IndexType guessIndex, int nonLetterMask) {
        //static std::unordered_map<int, int8_t> cache = {};
        //int myMask = nonLetterMask & letterCountLookup[guessIndex];
        //auto it = cache.find(myMask);
        //if (it != cache.end()) return it->second;

        int8_t r = 0;
        for (char c: GlobalState.reverseIndexLookup[guessIndex]) {
            bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
            r += knownNonLetter == true;
        }
        bool isPossibleAnswer = guessIndex < GlobalState.allAnswers.size();
        return 2*r + isPossibleAnswer;
    }

    static int getNodeId(IndexType guessIndex, int nonLetterMask) {
        // using MyPair = std::pair<IndexType, int>;
        // static std::map<MyPair, int> cache = {};
        // int myMask = nonLetterMask & letterCountLookup[guessIndex];
        // MyPair key = {guessIndex, myMask};
        // auto it = cache.find(key);
        // if (it != cache.end()) return it->second;

        auto trieIdUsingCompressed = getNodeIdUsingCompressed(guessIndex, nonLetterMask);
        //assert(trieIdUsingCompressed == getNodeIdUsingNormal(guessIndex, nonLetterMask));
        return trieIdUsingCompressed;

        auto repString = GlobalState.reverseIndexLookup[guessIndex];
        for (int i = 0; i < 5; ++i) {
            char c = GlobalState.reverseIndexLookup[guessIndex][i];
            bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
            if (knownNonLetter) {
                repString[i] = '.';
            }
        }
        //assert(NonLetterLookup::stringPatternToId.find(repString) != NonLetterLookup::stringPatternToId.end());
        auto actual = NonLetterLookup::idToStringPattern[trieIdUsingCompressed];
        if (actual != repString) {
            DEBUG("HELLO??  orig: " << GlobalState.reverseIndexLookup[guessIndex] << ", expected: " << repString << ", actual: " << actual << ", trieId: " << trieIdUsingCompressed); exit(1);
        }
        return NonLetterLookup::stringPatternToId[repString];
    }

    static int getNodeIdUsingCompressed(IndexType guessIndex, int nonLetterMask) {
        int trieId = guessIndex;
        for (uint8_t charIndex: NonLetterLookup::guessIndexToCharIndexes[guessIndex]) {
            bool knownNonLetter = (nonLetterMask & (1 << charIndex)) != 0;
            if (knownNonLetter) {
                trieId = NonLetterLookup::compressedTrieNodes[trieId].childByLetter[charIndex];
            }
        }
        return trieId;
    }

    static int getNodeIdUsingNormal(IndexType guessIndex, int nonLetterMask) {
        int trieId = guessIndex;
        for (int i = 0; i < 5; ++i) {
            char c = GlobalState.reverseIndexLookup[guessIndex][i];
            bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
            if (knownNonLetter) {
                trieId = NonLetterLookup::trieNodes[trieId].childByPosition[i];
            }
        }
        return trieId;
    }

    static void clearGuesses(GuessesVec &guesses, const AnswersVec &answers) {
        auto ratio = (double)guesses.size() / answers.size();
        
        if (ratio <= 2.00) {
            //DEBUG("skipping...");
            return;
        }
        int answerLetterMask = 0;
        for (auto answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        std::size_t deleted = 0;
        for (std::size_t i = 0; i < guesses.size() - deleted; ++i) {
            if ((answerLetterMask & letterCountLookup[guesses[i]]) == 0) {
                std::swap(guesses[i], guesses[guesses.size()-1-deleted]);
                deleted++;
                --i;
            }
        }
        guesses.resize(guesses.size() - deleted);
        //DEBUG("sending... " << ratio << " #guesses: " << guesses.size() << ", #answers: " << answers.size() << ", removed: " << deleted);
    }

    // clear guesses with equal or better guess...
    // looking at answerLetterMask

    static std::size_t clearGuesses(UnorderedVector<IndexType> &guesses, const UnorderedVector<IndexType> &answers) {
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

    static inline int specialMask = 0;
    static void buildLetterLookup() {
        if (letterCountLookup.size() > 0) return;
        letterCountLookup.resize(GlobalState.reverseIndexLookup.size());
        for (std::size_t i = 0; i < GlobalState.reverseIndexLookup.size(); ++i) {
            letterCountLookup[i] = 0;
            for (auto c: GlobalState.reverseIndexLookup[i]) {
                letterCountLookup[i] |= (1 << (c-'a'));
            }
        }
        for (auto c: GlobalArgs.specialLetters) {
            specialMask |= (1 << (c-'a'));
        }
    }
};
