#pragma once
#include "Defs.h"
#include "Util.h"

#include <vector>
#include <unordered_map>

struct SubsetCache {
    // subsetCache[remDepth][answerIndex] is a vector of "subsetIds", which are ids that had answerIndex in their cache
    std::vector<std::vector<std::vector<int>>> subsetCache = {};
    std::vector<int> minIdSize;
    std::vector<std::vector<int>> minIdSizes;
    std::vector<std::vector<IndexType>> idSize = {};
    std::vector<std::unordered_map<int, int>> numHits = {};

    SubsetCache(int maxTries) {
        minIdSize.assign(maxTries+1, INF_INT);
        minIdSizes.assign(maxTries+1, std::vector<int>(GlobalState.allAnswers.size(), INF_INT));
        subsetCache.resize(maxTries+1);
        numHits.resize(maxTries+1);
        for (int i = 0; i <= maxTries; ++i) {
            subsetCache[i].resize(GlobalState.allAnswers.size());
        }
        idSize.resize(maxTries+1);
    }

    void setSubsetCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        int oldLb, int lb)
    {
        if (lb == 0 || oldLb > 0) return;

        int id = idSize[remDepth].size();
        for (auto ans: answers) {
            subsetCache[remDepth][ans].push_back(id);
            minIdSizes[remDepth][ans] = std::min(minIdSizes[remDepth][ans], static_cast<int>(answers.size()));
        }
        idSize[remDepth].push_back(answers.size());
        minIdSize[remDepth] = std::min(minIdSize[remDepth], static_cast<int>(answers.size()));
    }

    int getLbFromAnswerSubsetsSLOW(
        std::vector<std::map<std::pair<AnswersVec, GuessesVec>, LookupCacheEntry>> &guessCache2,
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth)
    {

        static long long supersetChecks = 0;
        int lb = 0;
        auto &cache = guessCache2[remDepth];
        for (auto &[key, val]: cache) {
            if (val.lb == 0) continue;
            supersetChecks++;
            // if answers is a SUPERSET, the current problem is harder therefore the lb can be used
            auto answersIsSuperset = std::includes(answers.begin(), answers.end(), key.first.begin(), key.first.end());
            if (answersIsSuperset) {
                lb = 1;
                if (GlobalArgs.maxWrong == 0) break;
            }
        }

        static long long miss = 0, hit = 0;
        if (lb > 0) hit++;
        else miss++;
        if ((hit+miss) % 1000 == 0) DEBUG("status: " << getPerc(hit, hit+miss) << ", supersetChecks: " << supersetChecks);

        return lb;
    }


    inline static long long lbFromHarderInstances = 0;
    int getLbFromAnswerSubsetsFAST(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth)
    {
        // if answers is a SUPERSET, the current problem is harder therefore the lb can be used
        std::vector<IndexType> counts = idSize[remDepth];

        bool hasMatch = false;
        const bool doSort = false;

        IndexType m = minIdSize[remDepth];
        // int maxMFromAnswers = 0;
        // for (auto ans: answers) maxMFromAnswers = std::max(maxMFromAnswers, minIdSizes[remDepth][ans]);

        // if (maxMFromAnswers < m) {
        //     m = maxMFromAnswers;
        // }
        int nAnswers = answers.size();
        for (int i = 0; i < nAnswers; ++i) {
            int ans = answers[i];
            int answersLeft = nAnswers - i;
            if (m > answersLeft) break;
            if (hasMatch) break;
            lbFromHarderInstances++;

            auto &vec = subsetCache[remDepth][ans];
            if (doSort) {
                auto cmp = [&](int lhs, int rhs) { return idSize[remDepth][lhs] < idSize[remDepth][rhs]; };
                if (!std::is_sorted(vec.begin(), vec.end(), cmp)) {
                    std::sort(vec.begin(), vec.end(), cmp);
                }
            }

            for (auto id: vec) {
                auto r = --counts[id];
                if (r < m) {
                    m = r;
                    if (m == 0) {
                        hasMatch = true;
                        recordHit(remDepth, id);
                        break;
                    }
                }
                // assumes subsetCache[remDepth][ans] is in sorted order by idSize.
                //if (doSort && idSize[remDepth][id] > static_cast<int>(answers.size())) break;
            }
        }
        
        return hasMatch ? 1 : 0;
    }

    void recordHit(RemDepthType remDepth, int subsetId) {
        static bool recordingEnabled = true;
        if (!recordingEnabled) return;

        numHits[remDepth][subsetId]++; // trust me, its default initialised to zero
    }

    AnswersVec getAnswers(RemDepthType remDepth, int subsetId) const {
        AnswersVec res = {};
        auto answers = getVector(GlobalState.allAnswers.size());
        for (IndexType answerIndex: answers) {
            const auto &vec = subsetCache[remDepth][answerIndex];
            if (std::find(vec.begin(), vec.end(), subsetId) != vec.end()) {
                res.push_back(answerIndex);
            }
        }
        return res;
    }
};
