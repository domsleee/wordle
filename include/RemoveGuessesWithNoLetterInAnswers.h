#pragma once
#include "GlobalState.h"
#include "PerfStats.h"
#include "NonLetterLookup.h"

struct RemoveGuessesWithNoLetterInAnswers {
    inline static std::vector<int> letterCountLookup = {};

    /*static std::vector<IndexType> clearGuesses(std::vector<IndexType> guesses, const std::vector<IndexType> &answers) {   
        int answerLetterMask = 0;
        for (auto &answerIndex: answers) {
            answerLetterMask |= letterCountLookup[answerIndex];
        }
        std::erase_if(guesses, [&](const int &guessIndex) {
            return (answerLetterMask & letterCountLookup[guessIndex]) == 0;
        });
        return guesses;
    }&*/

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

            std::function<void(std::string, int)> yeet = [&](std::string s, int toMutate) {
                if (toMutate == 5) return;

                yeet(s, toMutate+1);
                if (s[toMutate] != '.') {
                    s[toMutate] = '.';
                    stringToFirstSeen.emplace(s, guessIndex); //steamroll
                    yeet(s, toMutate+1);
                }
            };
            stats.tick(4444);
            yeet(repString, 0);
            stats.tock(4444);
        }

        std::erase_if(guesses, [&](auto guessIndex) {
            return stringToFirstSeen[indexToRepString[guessIndex]] != guessIndex;
        });

        // computeGuessToNumGroups();
        //std::sort(guesses.begin(), guesses.end(), [&](auto a, auto b) { return guessToNumGroups[a] > guessToNumGroups[b];});        
    }

    static void removeWithBetterOrSameGuessFaster(PerfStats &stats, GuessesVec &guesses, const int nonLetterMask) {
        stats.tick(10);
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
        stats.tock(10);

        std::vector<int> guessIndexToNodeId(GlobalState.allGuesses.size());
        std::vector<int> nodeIdToFirstSeen(NonLetterLookup::nodes.size(), -1);
        std::stack<int> nodeIdsToMark = {};
        stats.tick(11);
        for (auto guessIndex: guesses) {
            auto nodeId = getNodeId(guessIndex, nonLetterMask);
            guessIndexToNodeId[guessIndex] = nodeId;

            if (nodeIdToFirstSeen[nodeId] != -1) continue;
            nodeIdToFirstSeen[nodeId] = guessIndex;

            //DEBUG("go: " << repString);
            nodeIdsToMark.push(nodeId);
            while (!nodeIdsToMark.empty()) {
                auto t = nodeIdsToMark.top(); nodeIdsToMark.pop();
                //DEBUG("child: " << NonLetterLookup::idToStringPattern[t] << ": (t: " << t << ") #childreN: " << NonLetterLookup::nodes[t].childIds.size());
                for (auto childId: NonLetterLookup::nodes[t].childIds) {
                    if (nodeIdToFirstSeen[childId] != -1) continue;
                    nodeIdToFirstSeen[childId] = guessIndex;
                    nodeIdsToMark.push(childId);
                }
            }
        }
        stats.tock(11);

        stats.tick(13);
        std::erase_if(guesses, [&](auto guessIndex) {
            return nodeIdToFirstSeen[guessIndexToNodeId[guessIndex]] != guessIndex;
        });
        stats.tock(13);
    }

    static int getNodeId(IndexType guessIndex, int nonLetterMask) {
        auto repString = GlobalState.reverseIndexLookup[guessIndex];
        for (int i = 0; i < 5; ++i) {
            char c = GlobalState.reverseIndexLookup[guessIndex][i];
            bool knownNonLetter = (nonLetterMask & (1 << (c-'a'))) != 0;
            if (knownNonLetter) {
                repString[i] = '.';
            }
        }
        assert(NonLetterLookup::stringPatternToId.find(repString) != NonLetterLookup::stringPatternToId.end());
        return NonLetterLookup::stringPatternToId[repString];
    }

    static inline std::vector<long long> guessToNumGroups = {};
    static void computeGuessToNumGroups() {
        if (guessToNumGroups.size() != 0) return;
        START_TIMER(computeGuessToNumGroups);
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        auto answers = getVector<GuessesVec>(GlobalState.allGuesses.size());
        guessToNumGroups.assign(guesses.size(), 0);
        for (auto guessIndex: guesses) {
            bool seen[243] = {0};
            for (auto answerIndex: answers) {
                auto s = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                if (seen[s]) continue;
                seen[s] = 1;
                guessToNumGroups[guessIndex]++;
            }
        }
        END_TIMER(computeGuessToNumGroups);
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
    // https://nerdschalk.com/what-are-most-common-letters-for-wordle/
    // eartolsincuyhpdmgbkfwvxzqj
    static inline std::string specialLetters = "eartolsincu";//pdmgbkfwvxzqj";// fpln "abcdefghijklmnopqrstuvwxyz";

    static void buildClearGuessesInfo() {
        if (letterCountLookup.size() > 0) return;
        letterCountLookup.resize(GlobalState.reverseIndexLookup.size());
        for (std::size_t i = 0; i < GlobalState.reverseIndexLookup.size(); ++i) {
            letterCountLookup[i] = 0;
            for (auto c: GlobalState.reverseIndexLookup[i]) {
                letterCountLookup[i] |= (1 << (c-'a'));
            }
        }
        for (auto c: specialLetters) {
            specialMask |= (1 << (c-'a'));
        }
    }
};
