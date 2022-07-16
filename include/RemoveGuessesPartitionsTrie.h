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
    std::map<int, int> next = {};
};

struct RemoveGuessesPartitionsTrie {
    static inline thread_local std::vector<PartitionsTrieNode> nodes = {};
    static void removeWithBetterOrSameGuess(PerfStats &stats, GuessesVec &_guesses, const AnswersVec &answers) {
        nodes.resize(0);
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

        getNewNodeId();
        for (auto t: guesses) {
            nodes[0].subset.push_back(t);
            for (const auto &p: partitions[t]) {
                int nodeId = 0;
                for (auto t2: p) {
                    auto it = nodes[nodeId].next.find(t2);
                    int nextNodeId;
                    if (it == nodes[nodeId].next.end()) {
                        nextNodeId = getNewNodeId();
                        nodes[nodeId].next.insert({t2, nextNodeId});
                    } else {
                        nextNodeId = it->second;
                    }
                    nodeId = nextNodeId;
                    //nodes[nodeId].subset.push_back(t);
                }
                //if (t == 751) DEBUG("ADD 751 to exact: " << nodeId);
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

        stats.tick(72);
        for (auto t: guesses) {
            if (eliminated[t]) continue;
            for (const auto &p: partitions[t]) {
                const int pSize = p.size();
                std::stack<std::pair<int,int>> st = {};
                st.push({0, 0});
                while (!st.empty()) {
                    auto [nodeId, i] = st.top();
                    if (nodes[nodeId].exact.size() > 0) nodes[nodeId].subset.push_back(t);
                    st.pop();
                    for (const auto entry: nodes[nodeId].next) {
                        while (i < pSize && p[i] < entry.first) i++;
                        if (i == pSize) break;
                        if (p[i] == entry.first) {
                            st.push({entry.second, i+1});
                        }
                    }
                    // for (int j = i; j < x; j++) {
                    //     auto it = nodes[nodeId].next.find(p[j]);
                    //     if (it != nodes[nodeId].next.end()) {
                    //         st.push({it->second, j+1});
                    //     }
                    // }
                }
            }
        }
        stats.tock(72);

        stats.tick(73);
        for (auto t1: guesses) {
            if (eliminated[t1]) continue;
            auto initialNodeId = getNodeId(partitions[t1][0]);
            auto exactVec = nodes[initialNodeId].exact;
            auto subsetVec = nodes[initialNodeId].subset;
            for (int i = 1; i < static_cast<int>(partitions[t1].size()); ++i) {
                auto nodeId = getNodeId(partitions[t1][i]);
                inplaceSetIntersection(exactVec, nodes[nodeId].exact);
                inplaceSetIntersection(subsetVec, nodes[nodeId].subset);
            }

            inplaceSetDifference(subsetVec, exactVec);
            for (auto t2: subsetVec) if (t1 != t2) eliminated[t2] = 1;
        }
        stats.tock(73);

        stats.tick(74);
        std::erase_if(_guesses, [&](const auto guessIndex) {
            return eliminated[guessIndex] == 1;
        });
        stats.tock(74);
    }

    static int getNodeId(const AnswersVec &p) {
        int nodeId = 0;
        for (auto t2: p) {
            nodeId = nodes[nodeId].next[t2];
        }
        return nodeId;
    }

    static int getNewNodeId() {
        nodes.push_back({});
        return nodes.size()-1;
    }

    template <typename T>
    static void inplaceSetIntersection(std::vector<T> &a, const std::vector<T> &b) {
        // trust me, its fine
        // https://stackoverflow.com/questions/1773526/in-place-c-set-intersection
        auto nd = std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end());
    }

    template <typename T>
    static void inplaceSetDifference(std::vector<T> &a, const std::vector<T> &b) {
        auto nd = std::set_difference(a.begin(), a.end(), b.begin(), b.end(), a.begin());
        a.erase(nd, a.end()); // I think its fine?
    }
};