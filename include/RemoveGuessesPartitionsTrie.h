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

struct CachedSetIntersection {
    const std::vector<PartitionsTrieNode> &nodes;
    std::map<std::pair<int, int>, std::vector<int>> cache = {};

    CachedSetIntersection(const std::vector<PartitionsTrieNode> &nodes)
        : nodes(nodes) {}

    void cachedInplaceSetIntersection(std::vector<int> &a, int n1, int n2) {
        int m = std::min(n1, n2);
        int M = std::max(n1, n2);
        std::pair<int,int> p(m, M);
        auto it = cache.find(p);
        if (it != cache.end()) {
            inplaceSetIntersection(a, it->second);
        } else {
            std::vector<int> r1 = nodes[n1].subset;
            inplaceSetIntersection(r1, nodes[n2].subset);
            cache[p] = r1;
            inplaceSetIntersection(a, r1);
        }
    }
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


struct PartitionMetaGroup {
    std::vector<std::vector<int>> indexes; // indexes[t] = indexes of meta for test word `t`
    std::vector<PartitionMeta> meta;
};

#define GET_NODE_ID(t1, i) metaGroup.meta[metaGroup.indexes[t1][i]].getNodeId<RemoveGuessesPartitionsTrie>()

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
        PartitionMetaGroup metaGroup = getPartitionsAndMeta(guesses, answers);
        //auto &[partitionMetaIndexes, partitionMeta] = getPartitionsAndMeta(guesses, answers);
        stats.tock(69);

        stats.tick(70);
        nodes.resize(0);
        getNewNodeId();
        for (const auto &meta: metaGroup.meta) {
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
            static auto lambda = [&](const int nodeId) -> const std::vector<int>& { return nodes[nodeId].exact; };
            const auto exactVec = getSetIntersection(metaGroup, lambda, t1);

            int minExact = exactVec.size() > 0 ? exactVec[0] : 0;
            for (auto t2: exactVec) if (originalOrder[t2] < originalOrder[minExact]) minExact = t2;
            for (auto t2: exactVec) {
                if (t1 == t2) continue;
                if (minExact != t2) eliminated[t2] = 1;
            }
        }
        stats.tock(71);

        stats.tick(75);
        for (auto &meta: metaGroup.meta) {
            std::erase_if(meta.ts, [&](const auto t) { return eliminated[t]; });
        }
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            std::erase_if(nodes[i].exact, [&](const auto t) { return eliminated[t]; });
        }
        stats.tock(75);

        stats.tick(72);
        std::stack<std::pair<int,int>> st = {};
        for (const auto &meta: metaGroup.meta) {
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
        auto cachedSetIntersection = CachedSetIntersection(nodes);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            const auto &pVec = metaGroup.indexes[t1];
            // auto exactVec = nodes[mNodeId].exact;
            enum SetIntersectionStrategy {
                Naive,
                Counting,
                AbuseSmallSets
            };

            std::vector<int> subsetVec = {};
            static auto lambda = [&](const int nodeId) -> const std::vector<int>& { return nodes[nodeId].subset; };
            const auto myStrat = SetIntersectionStrategy::Counting;
            if (myStrat == SetIntersectionStrategy::Naive) {
                subsetVec = getSetIntersection(metaGroup, lambda, t1);
            } else if (myStrat == SetIntersectionStrategy::AbuseSmallSets) {
                subsetVec = getSetIntersectionAbuseSmallSets(metaGroup, pVec, t1);
            } else if (myStrat == SetIntersectionStrategy::Counting) {
                subsetVec = getSetIntersectionCounting(metaGroup, lambda, t1);
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

    static std::vector<int> getSetIntersectionAbuseSmallSets(PartitionMetaGroup &metaGroup, const std::vector<int> &pVec, int t1) {
        //https://www.geeksforgeeks.org/intersection-of-n-sets
        const int pVecSize = pVec.size();
        std::vector<int> nodeIds(pVecSize, 0);
        int n0 = GET_NODE_ID(t1, 0);
        nodeIds[0] = n0;
        int minI = 0, minNodeSize = nodes[n0].subset.size();

        for (int i = 0; i < pVecSize; ++i) {
            const int ni = GET_NODE_ID(t1, i);
            nodeIds[i] = ni;
            const int sz = nodes[ni].subset.size();
            if (sz < minNodeSize) {
                minI = i, minNodeSize = sz;
            }
        }
        //std::sort(nodeIds.begin(), nodeIds.end(), [&](const auto a, const auto b) { return nodes[a].subset.size() < nodes[b].subset.size(); });

        std::vector<int> res = {};
        for (int t2: nodes[nodeIds[minI]].subset) {
            bool skip = false;
            for (int i = 0; i < pVecSize; ++i) {
                if (i == minI) continue;

                const int ni = nodeIds[i];
                if (!std::binary_search(nodes[ni].subset.begin(), nodes[ni].subset.end(), t2)) {
                    skip = true;
                    break;
                }
            }

            if (!skip) res.push_back(t2);
        }
        return res;
        //return getSetIntersection(metaGroup, pVec, t1);
    }

    template <typename T>
    static std::vector<int> getSetIntersectionCounting(PartitionMetaGroup &metaGroup, T const& getVectorFn, int t1) {
        const auto &pVec = metaGroup.indexes[t1];
        std::vector<int> res = {};
        res.reserve(pVec.size());
        std::vector<IndexType> ct(GlobalState.allAnswers.size(), 0);
        const int pVecSize = pVec.size();
        for (int i = 0; i < pVecSize; ++i) {
            auto nodeId = GET_NODE_ID(t1, i);
            for (const auto t2: getVectorFn(nodeId)) {
                if (++ct[t2] == pVecSize) res.push_back(t2);
            }
        }
        std::sort(res.begin(), res.end());
        return res;
    }

    template <typename T>
    static std::vector<int> getSetIntersection(PartitionMetaGroup &metaGroup, T const& getVectorFn, int t1) {
        const auto &pVec = metaGroup.indexes[t1];
        int mNodeId = GET_NODE_ID(t1, 0);
        std::vector<int> subsetVec = getVectorFn(mNodeId);

        const int pVecSize = pVec.size();
        for (int i = 1; i < pVecSize; ++i) {
            auto nodeId = GET_NODE_ID(t1, i);
            inplaceSetIntersection(subsetVec, getVectorFn(nodeId));
        }
        return subsetVec;
    }

    static PartitionMetaGroup getPartitionsAndMeta(const GuessesVec &guesses, const AnswersVec &answers) {
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

        auto metaGroup = PartitionMetaGroup();
        metaGroup.indexes = {};
        metaGroup.indexes.assign(GlobalState.allGuesses.size(), std::vector<int>());
        metaGroup.meta = {};
        std::map<AnswersVec, int> partitionToMetaIndex = {};
        // std::vector<std::vector<int>> partitionMetaIndexes();
        // std::vector<PartitionMeta> partitionMeta = {};
        for (auto t: guesses) {
            for (const auto &p: partitions[t]) {
                auto it = partitionToMetaIndex.find(p);
                int metaIndex;
                if (it == partitionToMetaIndex.end()) {
                    metaIndex = metaGroup.meta.size();
                    partitionToMetaIndex[p] = metaIndex;
                    metaGroup.meta.resize(metaIndex+1);
                } else {
                    metaIndex = it->second;
                }
                metaGroup.meta[metaIndex].ts.push_back(t);
                metaGroup.meta[metaIndex].p = p;
                metaGroup.indexes[t].push_back(metaIndex);
            }

            //auto &indexes = metaGroup.indexes[t];
            //std::sort(indexes.begin(), indexes.end(), [&](const auto &a, const auto &b) { return metaGroup.meta[a].p.size() < metaGroup.meta[b].p.size(); });
            //if (indexes.size() > 0) DEBUG("indexes[0]: " << metaGroup.meta[indexes[0]].p.size() << " VS " << metaGroup.meta[indexes[1]].p.size());
        }

        return metaGroup;
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
    static void inplaceSetDifference(std::vector<T> &a, const std::vector<T> &b) {
        auto nd = std::set_difference(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end()); // I think its fine?
    }
};
