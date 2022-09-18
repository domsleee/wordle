#pragma once
#include <vector>
#include <string>
#include "Util.h"
#include "PerfStats.h"
#include "PatternGetterCached.h"
#include "Defs.h"
#include "GlobalState.h"
#include "SolverHelper.h"

enum CompareResult {
    BetterThanOrEqualTo,
    Unsure
};

using PartitionVec = std::vector<std::vector<AnswersVec>>;
struct PartitionInfo {
    PartitionVec partitions = {};
    std::vector<int> sumAfterDeleted = {};
};

// O(H.T^2)
struct RemoveGuessesPartitions {
    using PartitionInvertedIndex = std::vector<int>;
    const int remDepth;

    RemoveGuessesPartitions(int remDepth): remDepth(remDepth) {}

    void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &guesses, const AnswersVec &answers) {
        //DEBUG("H: " << answers.size() << ", T: " << guesses.size());
        stats.tick(70);
        auto partitionInfo = getPartitionsINDEX(guesses, answers);
        stats.tock(70);
        //auto partitionInvertedIndex = buildPartitionInvertedIndex(partitions);

        stats.tick(72);
        bool hasZeroPartition = removePartitionsWithZeroOrOneParts(stats, guesses, partitionInfo, answers.size());
        if (hasZeroPartition) return;
        stats.tock(72);

        stats.tick(71);
        const int nGuesses = guesses.size();
        std::vector<int8_t> eliminated(nGuesses, 0);

        for (int i = 0; i < nGuesses; ++i) {
            bool requireReset = false;
            if (eliminated[i]) continue;
            for (int j = i+1; j < nGuesses; ++j) {
                if (eliminated[j]) continue;
                requireReset |= determineEliminate(guesses, eliminated, partitionInfo, i, j);
                if (eliminated[i]) break;
            }
            if (requireReset) i--;
        }
        stats.tock(71);

        int count = 0;
        std::erase_if(guesses, [&](const auto guessIndex) {
            ++count;
            return eliminated[count-1];
        });
    }

    bool determineEliminate(GuessesVec &guesses, std::vector<int8_t> &eliminated, PartitionInfo &partitionInfo, int i, int j) {
        assert(i < j);

        auto r1 = compare(partitionInfo, i, j);
        if (r1 == BetterThanOrEqualTo) {
            markAsEliminated(eliminated, j, i);
            return false;
        }

        auto r2 = compare(partitionInfo, j, i);
        if (r2 == BetterThanOrEqualTo) {
            //markAsEliminated(eliminated, i, j);
            std::swap(partitionInfo.partitions[i], partitionInfo.partitions[j]);
            std::swap(partitionInfo.sumAfterDeleted[i], partitionInfo.sumAfterDeleted[j]);
            std::swap(guesses[i], guesses[j]);
            markAsEliminated(eliminated, j, i);
            return true;
        }
        return false;
    }

    static PartitionInvertedIndex buildPartitionInvertedIndex(const PartitionVec &partitions) {
        const int nPartitions = partitions.size();
        const int allAnswersSize = GlobalState.allAnswers.size();
        PartitionInvertedIndex partitionInvertedIndex(nPartitions * GlobalState.allAnswers.size(), 0);
        for (int i = 0; i < nPartitions; ++i) {
            for (int pi = 0; pi < static_cast<int>(partitions[i].size()); ++pi) {
                const auto &p = partitions[i][pi];
                for (auto h1: p) {
                    partitionInvertedIndex[i * allAnswersSize + h1] = pi;
                }
            }
        }
        return partitionInvertedIndex;
    }

    static void markAsEliminated(std::vector<int8_t> &eliminated, IndexType guessToElim, IndexType guessThatIsBetter, bool equalTo = false) {
        // if (true || guessToElim == 751) {
        //     std::string op = equalTo ? std::string(">=") : std::string(">");
        //     DEBUG("eliminated " << guessToElim << " because " << guessThatIsBetter << ' ' << op << ' ' << guessToElim);
        // }
        eliminated[guessToElim] = 1;
    }

    static bool isCompletelyUselessPartition(const std::vector<AnswersVec> &p, const int nAnswers) {
        return p.size() == 1 && static_cast<int>(p[0].size()) == nAnswers;
    }

    static bool removePartitionsWithZeroOrOneParts(PerfStats &stats, GuessesVec &guesses, PartitionInfo &partitionInfo, const int nAnswers) {
        const int nGuesses = guesses.size();
        std::size_t maxNumPartitions = 0;
        for (int i = 0; i < nGuesses; ++i) {
            if (partitionInfo.partitions[i].size() == 0) return true;
            if (isCompletelyUselessPartition(partitionInfo.partitions[i], nAnswers)) continue;
            maxNumPartitions = std::max(maxNumPartitions, partitionInfo.partitions[i].size());
        }
        if (maxNumPartitions == 0) return true;
        int numDeleted = 0;
        for (int i = 0; i < nGuesses-numDeleted; ++i) {
            guesses[i] = guesses[i+numDeleted];
            partitionInfo.partitions[i].swap(partitionInfo.partitions[i+numDeleted]);
            partitionInfo.sumAfterDeleted[i] = partitionInfo.sumAfterDeleted[i+numDeleted];
            bool shouldDelete = isCompletelyUselessPartition(partitionInfo.partitions[i], nAnswers);
            if (shouldDelete) {
                i--;
                numDeleted++;
            }
        }
        guesses.resize(nGuesses - numDeleted);
        partitionInfo.partitions.resize(nGuesses - numDeleted);
        partitionInfo.sumAfterDeleted.resize(nGuesses - numDeleted);
        return false;
    }

    PartitionInfo getPartitionsINDEX(const GuessesVec &guesses, const AnswersVec &answers) {
        const int nGuesses = guesses.size();
        PartitionInfo partitionInfo = {};
        partitionInfo.partitions.assign(nGuesses, std::vector<AnswersVec>());
        partitionInfo.sumAfterDeleted.assign(nGuesses, 0);

        auto &partitions = partitionInfo.partitions;
        assert(std::is_sorted(answers.begin(), answers.end()));
        //const int H = answers.size(), T = guesses.size();
        //DEBUG("H: " << H << ", T: " << T);
        for (int i = 0; i < nGuesses; ++i) {
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guesses[i]);
                if (patternToInd[patternInt] == 255) {
                    const auto ind = partitions[i].size();
                    patternToInd[patternInt] = ind;
                    partitions[i].push_back({answerIndex});
                } else {
                    const auto ind = patternToInd[patternInt];
                    partitions[i][ind].push_back(answerIndex);
                }
            }
            std::erase_if(partitions[i], [&](const auto &p) -> bool {
                return safeToIgnorePartition(p.size());
            });
            std::stable_sort(partitions[i].begin(), partitions[i].end(), [](const auto &p1, const auto &p2) -> bool {return p1.size() < p2.size();});
            int sumAfterDeleted = 0;
            for (auto &p: partitions[i]) sumAfterDeleted += p.size();
            partitionInfo.sumAfterDeleted[i] = sumAfterDeleted;
            // std::vector<int> pSizes = {};
            // for (const auto &p: partitions[i]) {
            //     pSizes.push_back(p.size());
            // }
            // std::sort(pSizes.begin(), pSizes.end());
            // DEBUG(getIterableString(pSizes));
        }

        return partitionInfo;
    }

    // solved in 2: 3
    // solved in 3: 5 ==> {1,1,3}
    // solved in 4: 7 ==> {1,1,5}
    bool safeToIgnorePartition(int partitionSize) {
        assert(remDepth >= 3);
        const int lt = (remDepth - 3) + SolverHelper::getMaxGuaranteedSolvedInRemDepth(2);
        return (partitionSize <= lt); // assumes remDepth >= 2
    }

    // is g1 a better guess than g2
    // if all partitions p1 of P(H, g1) has a partition p2 of P(H, g2) where p1 is a subset of p2, then i is at least as good as j
    CompareResult compare(const PartitionInfo &partitionInfo, int i, int j, bool isDebug = false) {
        const auto &partitions = partitionInfo.partitions;
        const bool couldP1BeSubsetOfP2 = compareEarlyExit(partitionInfo, i, j);
        const bool debugFalseEarlyExits = false;
        if (!debugFalseEarlyExits && !couldP1BeSubsetOfP2) return CompareResult::Unsure;
        
        for (const auto &p1: partitions[i]) {
            if (safeToIgnorePartition(p1.size())) continue;
            bool hasSubset = false;
            for (const auto &p2: partitions[j]) {
                auto p1IsSubsetOfP2 = std::includes(p2.begin(), p2.end(), p1.begin(), p1.end());
                if (p1IsSubsetOfP2) {
                    hasSubset = true; break;
                }
            }

            if (!hasSubset) {
                if (isDebug) {
                    DEBUG("g1 " << i << " is not a better guess than g2 " << j << " because:");
                    printIterable(p1);
                    DEBUG("had no subset in ");
                    printPartitions(partitions, j);
                }
                return CompareResult::Unsure;
            }
        }
        if (debugFalseEarlyExits && !couldP1BeSubsetOfP2) {
            DEBUG("FALSE EARLY EXIT HERE...");
            const auto &pi = partitionInfo.partitions[i], &pj = partitionInfo.partitions[j];
            DEBUG("largestRule: " << pi.rbegin()->size() << " <= " << pj.rbegin()->size());  // the largest of pi must be a subset of something
            
            DEBUG(partitions[i].size() << " VS " << partitions[j].size());
            printPartitions(partitions, i);
            printPartitions(partitions, j);
            exit(1);
        }
        return CompareResult::BetterThanOrEqualTo;
    }

    static bool compareEarlyExit(const PartitionInfo &partitionInfo, int i, int j) {
        const auto &pi = partitionInfo.partitions[i], &pj = partitionInfo.partitions[j];
        const bool largestRule = pi.rbegin()->size() <= pj.rbegin()->size();  // the largest of pi must be a subset of something
        
        const auto sumi = partitionInfo.sumAfterDeleted[i], sumj = partitionInfo.sumAfterDeleted[j];
        const bool sumRule = sumi <= sumj;
        const bool lengthRule = sumi != sumj || pi.size() >= pj.size(); // each pj needs to have at least one partition using it
        const bool smallestRule = sumi != sumj || pi[0].size() <= pj[0].size(); // the smallest of pj must be used

        // if (!largestRule
        //     // || !sumRule
        //     // || !lengthRule
        // ) return CompareResult::Unsure;
        const bool couldP1BeSubsetOfP2 = largestRule && sumRule && lengthRule && smallestRule;
        return couldP1BeSubsetOfP2;
    }

    static void printPartitions(const PartitionVec &partitions, int i) {
        DEBUG("partitions of " << i);
        for (const auto &p: partitions[i]) {
            DEBUG("----");
            std::string r = "";
            for (auto &v: p) r += std::string(",") + std::to_string(v);
            DEBUG("[" << p.size () << "]: " << r);
        }
    }
};