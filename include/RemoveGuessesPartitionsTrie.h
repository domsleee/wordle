#pragma once
#include <vector>
#include <map>
#include <algorithm>
#include <queue>
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
    AnswersVec p = {};
    std::vector<int> ts = {};
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
    std::vector<std::vector<int>> metaIndexesForT; // indexes[t] = indexes of meta for test word `t`
    std::vector<PartitionMeta> meta;
};

#define GET_NODE_ID(t1, i) metaGroup.meta[metaGroup.metaIndexesForT[t1][i]].getNodeId<RemoveGuessesPartitionsTrie>()

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
        buildSubsetNodesFast(stats, metaGroup, eliminated);
        stats.tock(72);

        stats.tick(74);
        auto cachedSetIntersection = CachedSetIntersection(nodes);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            const auto &pVec = metaGroup.metaIndexesForT[t1];
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
        assert(_guesses.size() > 0);
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
        const auto &pVec = metaGroup.metaIndexesForT[t1];
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
        const auto &pVec = metaGroup.metaIndexesForT[t1];
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
        metaGroup.metaIndexesForT = {};
        metaGroup.metaIndexesForT.assign(GlobalState.allGuesses.size(), std::vector<int>());
        metaGroup.meta = {};
        std::map<AnswersVec, int> partitionToMetaIndex = {};
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
                metaGroup.metaIndexesForT[t].push_back(metaIndex);
            }
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

    static void buildSubsetNodesForDebug(PerfStats &stats, PartitionMetaGroup &metaGroup, const std::vector<int8_t> &eliminated) {
        buildSubsetNodes(stats, metaGroup, eliminated);
        auto slowNodes = nodes;
        for (auto n: nodes) n.subset = {};
        buildSubsetNodesFast(stats, metaGroup, eliminated);
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            const auto &slowSubset = slowNodes[i].subset;
            const auto &fastSubset = nodes[i].subset;
            if (slowSubset != fastSubset) {
                DEBUG("ERROR. slow (" << slowSubset.size() << ") vs fast (" << fastSubset.size() << ")");
                DEBUG(getIterableString(slowSubset));
                DEBUG(getIterableString(fastSubset));
                exit(1);
            }
        }
    }

    static void buildSubsetNodes(PerfStats &stats, PartitionMetaGroup &metaGroup, const std::vector<int8_t> &eliminated) {
        std::stack<std::pair<int,int>> st = {};
        for (const auto &meta: metaGroup.meta) {
            const int pSize = meta.p.size();
            st.push({0, 0});
            while (!st.empty()) {
                //stats.tick(85);
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
                //stats.tock(85);
            }
        }

        stats.tick(73);
        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            std::sort(nodes[i].subset.begin(), nodes[i].subset.end());
        }
        stats.tock(73);
    }

    struct BFSNode {
        int nodeId = 0;
        std::vector<std::pair<int,int>> metaGroupIdPartitionIndexes = {};
    };
    using MyMinHeap = std::priority_queue<int, std::vector<int>, std::greater<int>>;

    static void buildSubsetNodesFast(PerfStats &stats, PartitionMetaGroup &metaGroup, const std::vector<int8_t> &eliminated) {
        if (metaGroup.meta.size() == 0) return;

        std::queue<BFSNode> q = {};
        
        BFSNode initialNode = {};
        for (int i = 0; i < metaGroup.meta.size(); ++i) {
            initialNode.metaGroupIdPartitionIndexes.push_back({i, 0});
        }
        q.push(initialNode);
        const int nGuesses = GlobalState.allGuesses.size();
        //DEBUG("NEW SEARCH");

        while (!q.empty()) {
            auto &bfsNode = q.front(); 
            auto &node = nodes[bfsNode.nodeId];
            stats.tick(85);
            calcSubsets(node, bfsNode, metaGroup, nGuesses);
            stats.tock(85);

            // stats.tick(86);
            const int nKeys = node.nextKeys.size();
            for (int j = 0; j < nKeys; ++j) {
                BFSNode nextNode = {};
                const auto nextKey = node.nextKeys[j];

                for (auto &[metaGroupId, pIndex]: bfsNode.metaGroupIdPartitionIndexes) {
                    const int pSize = metaGroup.meta[metaGroupId].p.size();
                    while (pIndex < pSize && metaGroup.meta[metaGroupId].p[pIndex] < nextKey) pIndex++;
                    if (pIndex == pSize) continue;

                    if (metaGroup.meta[metaGroupId].p[pIndex] == nextKey) {
                        nextNode.metaGroupIdPartitionIndexes.emplace_back(std::pair(metaGroupId, pIndex+1));
                    }
                }

                if (nextNode.metaGroupIdPartitionIndexes.size() > 0) {
                    nextNode.nodeId = node.nextVals[j];
                    q.emplace(nextNode);
                }
            }
            q.pop();
            // stats.tock(86);
        }
    }

    static void calcSubsets(PartitionsTrieNode &node, const BFSNode &bfsNode, const PartitionMetaGroup &metaGroup, const int nGuesses) {
        static thread_local MyMinHeap minHeap = {};
        static thread_local std::vector<int8_t> bitset(nGuesses, 0);

        if (node.exact.size() > 0) {
            auto getFromMetaGroupId = [&](const int metaGroupId) -> const std::vector<int>& {
                //DEBUG("metaGroup.meta.size(): " << metaGroup.meta.size() << ", id: " << metaGroupId);
                return metaGroup.meta[metaGroupId].ts;
            };

            //DEBUG("set union of: " <<  bfsNode.metaGroupIdPartitionIndexes.size());
            auto subsetVec = std::vector<int>();
            trackCalcSubsetsSize(bfsNode.metaGroupIdPartitionIndexes.size());
            //subsetVec.reserve(nGuesses);
            if (bfsNode.metaGroupIdPartitionIndexes.size() == 1) {
                subsetVec = getFromMetaGroupId(bfsNode.metaGroupIdPartitionIndexes[0].first);
            }
            else if (bfsNode.metaGroupIdPartitionIndexes.size() == 2)
            {
                const auto &ts1 = getFromMetaGroupId(bfsNode.metaGroupIdPartitionIndexes[0].first);
                const auto &ts2 = getFromMetaGroupId(bfsNode.metaGroupIdPartitionIndexes[1].first);
                std::set_union(ts1.begin(), ts1.end(), ts2.begin(), ts2.end(), std::back_inserter(subsetVec));
            }
            else if (bfsNode.metaGroupIdPartitionIndexes.size() < 500000) {
                subsetVec = getSubsetVecByUnionOfNSets_SmallHeap(bfsNode, metaGroup);
            }
            else if (bfsNode.metaGroupIdPartitionIndexes.size() < 2000) {
                int numMetaGroupIdIndexes = bfsNode.metaGroupIdPartitionIndexes.size();
                std::vector<const std::vector<int>*> ptrs(numMetaGroupIdIndexes);
                for (int i = 0; i < numMetaGroupIdIndexes; ++i) {
                    ptrs[i] = &getFromMetaGroupId(bfsNode.metaGroupIdPartitionIndexes[i].first);
                }
                std::vector<int> indexes(numMetaGroupIdIndexes, 0);
                const int nGuesses = GlobalState.allGuesses.size();

#define MERGE_DEBUG(x)
                MERGE_DEBUG("GOING: " << numMetaGroupIdIndexes);
                // for (int i = 0; i < numMetaGroupIdIndexes; ++i) {
                //     const auto &vec = *(ptrs[i]);
                //     MERGE_DEBUG(getIterableString(vec));
                // }
                int minI = 0, maxI = numMetaGroupIdIndexes-1;
                while (true) {
                    int m1 = nGuesses, m2 = nGuesses, ind1 = -1; 
                    for (int i = minI; i <= maxI; ++i) {
                        const auto &vec = *(ptrs[i]);
                        const int m = indexes[i] >= vec.size()
                            ? nGuesses
                            : vec[indexes[i]];
                        MERGE_DEBUG("CHECK m from ptrs[" << i << "]: " << m);
                        if (m < m1) {
                            m1 = m; ind1 = i;
                            MERGE_DEBUG("new m1!: " << m1 << ", from ptrs[" << i << "][" << indexes[i] << "]");
                        } else if (m < m2) {
                            m2 = m;
                        }
                    }
                    MERGE_DEBUG("ind1: " << ind1 << ", indexes[ind1]: " << indexes[ind1] << ", m1: " << m1 << ", m2: " << m2);
                    if (ind1 == -1) break;
                    const auto &ind1Vec = *(ptrs[ind1]);
                    MERGE_DEBUG("indexes[ind1]: " << indexes[ind1] << " < ind1Vec.size(): " << ind1Vec.size() << " && ind1Vec[ind1]: " << ind1Vec[ind1] << " <= m2: " << m2);
                    for (;indexes[ind1] < ind1Vec.size() && ind1Vec[indexes[ind1]] <= m2; ++indexes[ind1]) {
                        const int i = indexes[ind1];
                        if (subsetVec.size() == 0 || ind1Vec[i] != *subsetVec.rbegin()) {
                            subsetVec.push_back(ind1Vec[i]);
                        }
                    }
                    if (indexes[ind1] == ind1Vec.size()) {
                        if (ind1 == minI) {
                            // while (minI <= maxI) {
                            //     const auto &minIVec = *(ptrs[minI]);
                            //     if (indexes[minI] == minIVec.size()) minI++;
                            //     else break;
                            // }
                            minI++;
                        }
                        else if (ind1 == maxI) maxI--;
                    }
                }
                MERGE_DEBUG("done");
                //DEBUG(getIterableString(subsetVec));
            }
            else if (bfsNode.metaGroupIdPartitionIndexes.size() > 2000) {
                //std::fill(bitset.begin(), bitset.end(), 0);
                int m = GlobalState.allGuesses.size(), M = 0;
                for (const auto &[metaGroupId, partitionIndex]: bfsNode.metaGroupIdPartitionIndexes) {
                    const auto &ts = getFromMetaGroupId(metaGroupId);
                    for (auto v: ts) {
                        bitset[v] = 1;
                    }
                    m = std::min(m, *ts.begin());
                    M = std::max(M, *ts.rbegin());
                }

                moveFromBitsetToVal(subsetVec, bitset, m, M);
            }
            else if (true) {
                for (const auto &[metaGroupId, partitionIndex]: bfsNode.metaGroupIdPartitionIndexes) {
                    for (auto v: getFromMetaGroupId(metaGroupId)) minHeap.push(v);
                }
                int last = -1;
                while (!minHeap.empty()) {
                    if (last != minHeap.top()) {
                        subsetVec.push_back(minHeap.top());
                        last = minHeap.top();
                    }
                    minHeap.pop();
                }
            } else if (false) {
                std::set<int> mySet = {};
                for (const auto &[metaGroupId, partitionIndex]: bfsNode.metaGroupIdPartitionIndexes) {
                    for (auto v: getFromMetaGroupId(metaGroupId)) mySet.insert(v);
                }
                subsetVec = {mySet.begin(), mySet.end()};
            } else {
                for (const auto &[metaGroupId, partitionIndex]: bfsNode.metaGroupIdPartitionIndexes) {
                    inplaceSetUnion(subsetVec, getFromMetaGroupId(metaGroupId));
                }
            }
            
            node.subset.swap(subsetVec);
        }
    }

    static std::vector<int> getSubsetVecByUnionOfNSets_SmallHeap(const BFSNode &bfsNode, const PartitionMetaGroup &metaGroup) {
        auto getFromMetaGroupId = [&](const int metaGroupId) -> const std::vector<int>& {
            return metaGroup.meta[metaGroupId].ts;
        };

        auto subsetVec = std::vector<int>();
        int numMetaGroupIdIndexes = bfsNode.metaGroupIdPartitionIndexes.size();
        std::vector<const std::vector<int>*> metaGroupIdToTs(numMetaGroupIdIndexes);
        for (int i = 0; i < numMetaGroupIdIndexes; ++i) {
            metaGroupIdToTs[i] = &getFromMetaGroupId(bfsNode.metaGroupIdPartitionIndexes[i].first);
        }

        using MyPair = std::pair<const std::vector<int>*,int>;
        auto cmp = [&](const MyPair &a, const MyPair &b) {
            return (*a.first)[a.second] < (*b.first)[b.second];
        };
        std::priority_queue<MyPair, std::vector<MyPair>, decltype(cmp)> minHeap(cmp);
        for (int i = 0; i < numMetaGroupIdIndexes; ++i) {
            minHeap.push({metaGroupIdToTs[i], 0});
        }

        int last = -1;
        while (minHeap.size()) {
            auto topVal = minHeap.top(); minHeap.pop();

            auto v = (*topVal.first)[topVal.second];
            if (v != last) {
                subsetVec.push_back(v);
                last = v;
            }

            auto nextInd = topVal.second + 1;
            if (nextInd != (*topVal.first).size()) {
                minHeap.push({topVal.first, nextInd});
            }
        }

        return subsetVec;

    }

    static const long long NUM_RECORD = 50;
    static inline std::array<long long, NUM_RECORD> cts = {};
    static inline long long gt500 = 0;
    static inline long long total = 0;
    static void trackCalcSubsetsSize(std::size_t sz) {  
        ++total;
        if (sz < NUM_RECORD) cts[sz]++;
        else gt500++;
    }

    static void printCalcSubsetsSizeInfo() {
        const auto printEntry = [&](std::string name, long long v, long long cumulative) {
            printf("%4s: %10s %10s\n", name.c_str(), getPerc(v, total).c_str(), getPerc(cumulative, total).c_str());
        };
        long long cumulative = 0;
        for (int i = 0; i < NUM_RECORD; ++i) {
            if (cts[i] > 0) {
                cumulative += cts[i];
                printEntry(std::to_string(i), cts[i], cumulative);
            }
        }
        printEntry(std::string(">") + std::to_string(NUM_RECORD), gt500, cumulative);
    }

    template <typename T, typename U>
    static void moveFromBitsetToVal(std::vector<T> &vec, std::vector<U> &bitset, T m, T M) {
        for (int i = m; i <= M; ++i) {
            if (bitset[i]) {
                vec.emplace_back(i);
                bitset[i] = 0;
            }
        }
    }

};
