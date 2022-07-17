#pragma once
#include <vector>
#include <map>
#include <algorithm>
#include "Util.h"
#include "PerfStats.h"
#include "PatternGetterCached.h"
#include "Defs.h"
#include "GlobalState.h"
#include "RemoveGuessesPartitions.h"

struct PartitionsTrieNode {
    std::vector<int> subset = {}; // trie(p') = {t | exists(p\in P(H,t))(p' subseteq p)}
    std::vector<int> exact = {};
    //std::map<int, int> next = {};
    std::vector<int> nextKeys = {}, nextVals = {};
};

struct PartitionMeta {
    std::vector<int> ts = {};
    AnswersVec p = {};
    int nodeId = -1;

    template <typename T>
    int getNodeId() {
        if (nodeId != -1) {
            return nodeId;
        }
        return nodeId = T::getNodeId(p);
    }
};

#define GET_NODE_ID(t1, i) partitionMeta[partitionMetaIndexes[t1][i]].getNodeId<RemoveGuessesPartitionsTrie>()

struct RemoveGuessesPartitionsTrie {
    static inline thread_local std::vector<PartitionsTrieNode> nodes = {};
    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &_guesses, const AnswersVec &answers) {
        // static thread_local int called = 0;
        // DEBUG("CALLED" << ++called);

        stats.tick(75);
        auto guesses = _guesses;
        const int nGuesses = guesses.size();

        std::vector<int> originalOrder(GlobalState.allGuesses.size(), 0);
        for (int i = 0; i < nGuesses; ++i) {
            originalOrder[guesses[i]] = i;
        }

        std::sort(guesses.begin(), guesses.end());
        stats.tock(75);

        stats.tick(69);
        auto [partitionMetaIndexes, partitionMeta] = getPartitionsAndMeta(guesses, answers);
        stats.tock(69);

        stats.tick(70);
        nodes.resize(0);
        getNewNodeId();
        for (const auto &meta: partitionMeta) {
            int nodeId = 0;
            for (auto t2: meta.p) {
                auto ind = nodeNextFindInd(nodeId, t2);
                int nextNodeId;
                if (ind == static_cast<int>(nodes[nodeId].nextKeys.size())) {
                    nextNodeId = getNewNodeId();
                    nodeNextInsert(nodeId, t2, nextNodeId);
                } else {
                    nextNodeId = nodes[nodeId].nextVals[ind];
                }
                nodeId = nextNodeId;
            }
            nodes[nodeId].exact = meta.ts;
            //inplaceSetUnion(nodes[nodeId].exact, meta.ts);
            //nodes[nodeId].exact.push_back(t);
        }
        stats.tock(70);

        stats.tick(71);
        std::vector<int8_t> eliminated(GlobalState.allGuesses.size(), 0);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            auto initialNodeId = GET_NODE_ID(t1, 0);// getNodeId(partitions[t1][0]);
            auto exactVec = nodes[initialNodeId].exact;
            for (int i = 1; i < static_cast<int>(partitionMetaIndexes[t1].size()); ++i) {
                auto nodeId = GET_NODE_ID(t1, i); //  getNodeId(partitions[t1][i]);
                inplaceSetIntersection(exactVec, nodes[nodeId].exact);
            }

            int minExact = exactVec.size() > 0 ? exactVec[0] : 0;
            for (auto t2: exactVec) if (originalOrder[t2] < originalOrder[minExact]) minExact = t2;
            for (auto t2: exactVec) {
                if (t1 == t2) continue;
                if (minExact != t2) eliminated[t2] = 1;
            }
        }
        stats.tock(71);

        stats.tick(75);
        for (auto &meta: partitionMeta) {
            std::erase_if(meta.ts, [&](const auto t) { return eliminated[t]; });
        }
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            std::erase_if(nodes[i].exact, [&](const auto t) { return eliminated[t]; });
        }
        stats.tock(75);

        stats.tick(72);
        std::stack<std::pair<int,int>> st = {};
        for (const auto &meta: partitionMeta) {
            const int pSize = meta.p.size();
            st.push({0, 0});
            while (!st.empty()) {
                auto [nodeId, i] = st.top();
                auto &node = nodes[nodeId];
                if (node.exact.size() > 0) {
                    for (auto t: meta.ts) {
                        assert(!eliminated[t]);
                        node.subset.push_back(t);
                    }
                }
                st.pop();
            
                const int nKeys = node.nextKeys.size();
                for (int j = 0; j < nKeys; ++j) {
                    auto nextKey = node.nextKeys[j];
                    while (i < pSize && meta.p[i] < nextKey) i++;
                    if (i == pSize) break;
                   
                    nextKey = node.nextKeys[j];
                    if (meta.p[i] == nextKey) {
                        const auto nextVal = node.nextVals[j];
                        st.push({nextVal, i+1});
                        ++i;
                    }
                }
            }
        }
        stats.tock(72);

        stats.tick(73);
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            std::sort(nodes[i].subset.begin(), nodes[i].subset.end());
        }
        stats.tock(73);

        stats.tick(74);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            const auto &pVec = partitionMetaIndexes[t1];
            int mNodeId = GET_NODE_ID(t1, 0), mIndex = 0;

            auto exactVec = nodes[mNodeId].exact;
            auto subsetVec = nodes[mNodeId].subset;

            //inplaceSetDifference(exactVec, {t1});
            //inplaceSetDifference(subsetVec, {t1});

            for (int i = 0; i < static_cast<int>(pVec.size()); ++i) {
                if (i == mIndex) continue;
                auto nodeId = GET_NODE_ID(t1, i); // getNodeId(pVec[i]);
                //inplaceSetIntersection(exactVec, nodes[nodeId].exact);
                //DEBUG("INTERSECTING: " << subsetVec.size() << " with " << nodes[nodeId].subset.size());
                inplaceSetIntersection(subsetVec, nodes[nodeId].subset);
            }

            // inplaceSetDifference(subsetVec, exactVec);
            for (auto t2: subsetVec) if (t1 != t2) eliminated[t2] = 1;
        }
        stats.tock(74);

        stats.tick(75);
        std::erase_if(_guesses, [&](const auto guessIndex) {
            return eliminated[guessIndex] == 1;
        });
        stats.tock(75);
    }

    static std::pair<std::vector<std::vector<int>>, std::vector<PartitionMeta>>  getPartitionsAndMeta(const GuessesVec &guesses, const AnswersVec &answers) {
        RemoveGuessesPartitions::PartitionVec partitions(GlobalState.allGuesses.size(), std::vector<AnswersVec>());
        assert(std::is_sorted(answers.begin(), answers.end()));
        for (const auto t: guesses) {
            std::array<uint8_t, NUM_PATTERNS> patternToInd;
            patternToInd.fill(255);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, t);
                if (patternToInd[patternInt] == 255) {
                    const auto ind = partitions[t].size();
                    patternToInd[patternInt] = ind;
                    partitions[t].push_back({answerIndex});
                } else {
                    const auto ind = patternToInd[patternInt];
                    partitions[t][ind].push_back(answerIndex);
                }
            }
        }

        std::map<AnswersVec, int> partitionToMetaIndex = {};
        std::vector<std::vector<int>> partitionMetaIndexes(GlobalState.allGuesses.size(), std::vector<int>());
        std::vector<PartitionMeta> partitionMeta = {};
        for (auto t: guesses) {
            for (const auto &p: partitions[t]) {
                auto it = partitionToMetaIndex.find(p);
                int metaIndex;
                if (it == partitionToMetaIndex.end()) {
                    metaIndex = partitionMeta.size();
                    partitionToMetaIndex[p] = metaIndex;
                    partitionMeta.resize(metaIndex+1);
                } else {
                    metaIndex = it->second;
                }
                partitionMeta[metaIndex].ts.push_back(t);
                partitionMeta[metaIndex].p = p;
                partitionMetaIndexes[t].push_back(metaIndex);
            }
        }

        return {partitionMetaIndexes, partitionMeta};
    }

    static int getNodeId(const AnswersVec &p) {
        int nodeId = 0;
        for (auto t2: p) {
            nodeId = nodes[nodeId].nextVals[nodeNextFindInd(nodeId, t2)];
        }
        return nodeId;
    }

    static int getNewNodeId() {
        nodes.push_back({});
        return nodes.size()-1;
    }

    static void nodeNextInsert(int nodeId, int k, int v) {
        auto it = std::lower_bound(nodes[nodeId].nextKeys.begin(), nodes[nodeId].nextKeys.end(), k);
        it = nodes[nodeId].nextKeys.insert(it, k);
        auto &node = nodes[nodeId];
        const auto distOfIt = std::distance(node.nextKeys.begin(), it);
        const auto nextValsIt = node.nextVals.begin() + distOfIt;
        node.nextVals.insert(nextValsIt, v);
    }

    static int nodeNextFindInd(int nodeId, int k) {
        const auto &keys = nodes[nodeId].nextKeys;
        auto it = std::lower_bound(keys.begin(), keys.end(), k);
        if (it == keys.end() || *it != k) return keys.size();
        return std::distance(keys.begin(), it);
    }


    template <typename T>
    static void inplaceSetIntersection(std::vector<T> &a, const std::vector<T> &b) {
        // trust me, its fine
        // https://stackoverflow.com/questions/1773526/in-place-c-set-intersection
        //DEBUG("INTERSECTING: " << a.size() << " WITH " << b.size());
        auto nd = std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end());
    }

    template <typename T>
    static void inplaceSetUnion(std::vector<T> &a, const std::vector<T> &b) {
        // trust me, its fine
        // https://stackoverflow.com/questions/1773526/in-place-c-set-intersection
        //DEBUG("INTERSECTING: " << a.size() << " WITH " << b.size());
        std::vector<T> out = {};
        std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
        a.swap(out);
        // a.assign(out.begin(), out.end());
    }

    template <typename T>
    static void inplaceSetDifference(std::vector<T> &a, const std::vector<T> &b) {
        auto nd = std::set_difference(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end()); // I think its fine?
    }
};