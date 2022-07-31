#pragma once
#include <vector>
#include <string>
#include "Util.h"
#include "PerfStats.h"
#include "PatternGetterCached.h"
#include "Defs.h"
#include "GlobalState.h"
#include "RemoveGuessesPartitions.h"

struct RemoveGuessesPartitionsEqualOnly {
    using PartitionVec = std::vector<std::vector<AnswersVec>>;
    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &guesses, const AnswersVec &answers) {

        static int totalCalls = 0;
        ++totalCalls;

        PartitionVec partitions = getPartitions(guesses, answers);
        int nGuesses = guesses.size();
        
        //DEBUG("#answers: " << answers.size());
        std::map<AnswersVec, int> partitionIdMap = {};
        std::vector<std::vector<int>> partitionsUsingIds = {};
        for (int i = 0; i < static_cast<int>(partitions.size()); ++i) {
            std::vector<int> partitionIds = {};
            partitionIds.reserve(partitions[i].size());
            for (auto &p: partitions[i]) {
                int partitionId = -1;
                auto it = partitionIdMap.find(p);
                if (it == partitionIdMap.end()) {
                    partitionId = partitionIdMap.size();
                    partitionIdMap[p] = partitionId;
                } else {
                    partitionId = it->second;
                }
                partitionIds.push_back(partitionId);
            }
            std::sort(partitionIds.begin(), partitionIds.end());

            partitionsUsingIds.emplace_back(std::move(partitionIds));
        }

        auto indexes = getVector<std::vector<int>>(nGuesses);
        std::sort(indexes.begin(), indexes.end(), [&](auto i, auto j) -> bool {
            if (partitions[i].size() != partitions[j].size()) {
                return partitions[i].size() < partitions[j].size();
            }

            if (partitionsUsingIds[i] != partitionsUsingIds[j]) {
                return partitionsUsingIds[i] < partitionsUsingIds[j];
            }

            return i < j;
        });

        int count = 0;
        std::erase_if(guesses, [&](const auto guessIndex) {
            if (count == nGuesses-1) return false;
            ++count;
            return partitionsUsingIds[count-1] == partitionsUsingIds[count];
        });
    }

    static PartitionVec getPartitions(const GuessesVec &guesses, const AnswersVec &answers) {
        PartitionVec partitions(guesses.size(), std::vector<AnswersVec>());
        assert(std::is_sorted(answers.begin(), answers.end()));
        for (std::size_t i = 0; i < guesses.size(); ++i) {
            auto guessIndex = guesses[i];
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                if (patternToInd[patternInt] == 255) {
                    const auto ind = partitions[i].size();
                    patternToInd[patternInt] = ind;
                    partitions[i].push_back({answerIndex});
                } else {
                    const auto ind = patternToInd[patternInt];
                    partitions[i][ind].push_back(answerIndex);
                }
            }
        }

        return partitions;
    }
};