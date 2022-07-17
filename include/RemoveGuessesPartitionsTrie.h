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

struct RemoveGuessesPartitionsTrie {
    static inline thread_local std::vector<PartitionsTrieNode> nodes = {};
    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &_guesses, const AnswersVec &answers) {
        // static thread_local int called = 0;
        // DEBUG("CALLED" << ++called);

        RemoveGuessesPartitions::PartitionVec partitions = RemoveGuessesPartitions::getPartitions(_guesses, answers);

        auto guesses = _guesses;
        const int nGuesses = guesses.size();

        static std::vector<int> originalOrder(GlobalState.allGuesses.size(), 0);
        for (int i = 0; i < nGuesses; ++i) {
            originalOrder[guesses[i]] = i;
        }

        std::sort(guesses.begin(), guesses.end());

        //RemoveGuessesPartitions::printPartitions(partitions, 751);
        //RemoveGuessesPartitions::printPartitions(partitions, 9611);

        stats.tick(70);
        nodes.resize(0);
        getNewNodeId();
        for (auto t: guesses) {
            nodes[0].subset.push_back(t);
            for (const auto &p: partitions[t]) {
                int nodeId = 0;
                for (auto t2: p) {
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
                nodes[nodeId].exact.push_back(t);
            }
        }
        stats.tock(70);

        stats.tick(71);
        std::vector<int8_t> eliminated(GlobalState.allGuesses.size(), 0);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            auto initialNodeId = getNodeId(partitions[t1][0]);
            auto exactVec = nodes[initialNodeId].exact;
            for (int i = 1; i < static_cast<int>(partitions[t1].size()); ++i) {
                auto nodeId = getNodeId(partitions[t1][i]);
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

        stats.tick(70);
        nodes.resize(0);
        getNewNodeId();
        for (auto t: guesses) {
            if (eliminated[t]) continue;
            nodes[0].subset.push_back(t);
            for (const auto &p: partitions[t]) {
                int nodeId = 0;
                for (auto t2: p) {
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
                nodes[nodeId].exact.push_back(t);
            }
        }
        stats.tock(70);

        std::map<AnswersVec, std::vector<int>> partitionsToT = {};
        int totalP = 0;
        for (auto t: guesses) {
            if (eliminated[t]) continue;
            for (const auto &p: partitions[t]) { partitionsToT[p].push_back(t); totalP++; }
        }
        //DEBUG("potensh? " << getPerc(partitionsSet.size(), totalP));

        stats.tick(72);
        for (const auto &[p, vecT]: partitionsToT) {
            const int pSize = p.size();
            std::stack<std::pair<int,int>> st = {};
            st.push({0, 0});
            while (!st.empty()) {
                auto [nodeId, i] = st.top();
                if (nodes[nodeId].exact.size() > 0) {
                    for (auto t: vecT) if (!eliminated[t]) nodes[nodeId].push_back(t);
                }
                st.pop();
            
                for (int j = 0; j < static_cast<int>(nodes[nodeId].nextKeys.size()); ++j) {
                    auto nextKey = nodes[nodeId].nextKeys[j];
                    auto nextVal = nodes[nodeId].nextVals[j];
                
                    while (i < pSize && p[i] < nextKey) i++;
                    if (i == pSize) break;
                    if (p[i] == nextKey) {
                        st.push({nextVal, i+1});
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
            const auto &pVec = partitions[t1];
            int mNodeId = getNodeId(pVec[0]), mIndex = 0;
            for (int i = 1; i < static_cast<int>(pVec.size()); ++i) {
                auto nodeId = getNodeId(pVec[i]);
                if (nodes[nodeId].subset.size() < nodes[mNodeId].subset.size()) {
                    mIndex = i, mNodeId = nodeId;
                }
            }
            auto exactVec = nodes[mNodeId].exact;
            auto subsetVec = nodes[mNodeId].subset;

            for (int i = 0; i < static_cast<int>(pVec.size()); ++i) {
                if (i == mIndex) continue;
                auto nodeId = getNodeId(pVec[i]);
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

    static int getNodeId(const AnswersVec &p) {
        int nodeId = 0;
        for (auto t2: p) {
            ;
            nodeId = nodes[nodeId].nextVals[nodeNextFindInd(nodeId, t2)];
        }
        return nodeId;
    }

    static int getNewNodeId() {
        nodes.push_back({});
        return nodes.size()-1;
    }

    static void nodeNextInsert(int nodeId, int k, int v) {
        // if (nodeId == 0 && k == 3703) {
        //     auto ind = nodeNextFindInd(nodeId, k);
        //     int nSize = nodes[nodeId].nextKeys.size();
        //     DEBUG("nodes[" << nodeId << "].next[" << k << "] = " << v << " (existing ind: " << ind << " / " << nSize);
        //     if (ind != nSize) {
        //         DEBUG("who do you think you are?");
        //         exit(1);
        //     }
        // }

        auto it = std::lower_bound(nodes[nodeId].nextKeys.begin(), nodes[nodeId].nextKeys.end(), k);
        it = nodes[nodeId].nextKeys.insert(it, k);
        // if (!std::is_sorted(nodes[nodeId].nextKeys.begin(), nodes[nodeId].nextKeys.end())) {
        //     DEBUG("WHAT");
        //     exit(1);
        // }
        auto distOfIt = std::distance(nodes[nodeId].nextKeys.begin(), it);
        auto nextValsIt = nodes[nodeId].nextVals.begin() + distOfIt;
        nodes[nodeId].nextVals.insert(nextValsIt, v);
    }

    static int nodeNextFindInd(int nodeId, int k) {
        auto it = std::lower_bound(nodes[nodeId].nextKeys.begin(), nodes[nodeId].nextKeys.end(), k);
        if (it == nodes[nodeId].nextKeys.end() || *it != k) return nodes[nodeId].nextKeys.size();
        return std::distance(nodes[nodeId].nextKeys.begin(), it);
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
    static void inplaceSetDifference(std::vector<T> &a, const std::vector<T> &b) {
        auto nd = std::set_difference(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end()); // I think its fine?
    }
};