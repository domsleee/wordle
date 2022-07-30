#pragma once
#include "Defs.h"
#include "Util.h"

#include <queue>
#include <unordered_map>
#include <vector>
#include <map>

#define TRIE_DEBUG(x) 

struct BiTrieNode {
    std::map<int, int> next = {};
    int lb = 0;
};

struct BiTrieSubsetCache {
    std::vector<BiTrieNode> nodes;
    std::map<int, std::vector<int>> finishNodes;
    const int _remDepth;
    const int rootNodeId = 0;

    BiTrieSubsetCache(int remDepth): _remDepth(remDepth) {
        getNewNode();
    }

    void setSubsetCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        int oldLb, int lb)
    {
        if (lb == 0 || oldLb > 0) return;

        int nodeId = rootNodeId;
        for (auto ans: answers) {
            auto it = nodes[nodeId].next.find(ans);
            int nextNodeId = -1;
            if (it == nodes[nodeId].next.end()) {
                nextNodeId = getNewNode();
                TRIE_DEBUG("INSERT EDGE on " << nodeId << " -> " << nextNodeId << " from ans: " << ans);
                nodes[nodeId].next[ans] = nextNodeId;
                //nodes[nextNodeId].prev[ans] = nodeId;
            } else {
                nextNodeId = it->second;
            }
            nodeId = nextNodeId;
        }
        nodes[nodeId].lb = lb;
        //finishNodes[answers.back()].push_back(nodeId);
    }

    int getLbFromAnswerSubsets(const AnswersVec &answers,
        const GuessesVec &guesses) const
    {
        std::queue<std::pair<int,int>> q;

        int lb = 0;
        const int nAnswers = answers.size();
        q.push({0, rootNodeId});
        while (!q.empty()) {
            const auto [answerIndex, nodeIndex] = q.front(); q.pop();
            const auto &node = nodes[nodeIndex];
            lb = std::max(lb, node.lb);
            if (lb > 0) break;
            if (answerIndex == nAnswers) continue;

            TRIE_DEBUG("INSPECTING NODE: " << nodeIndex << " #edges: " << node.next.size());
            const auto numToCheck = nAnswers - answerIndex;
            const auto rat = node.next.size() / numToCheck;
            if (rat < 20) {
                int i = answerIndex;
                for (const auto entry: node.next) {
                    while (i < nAnswers && answers[i] < entry.first) i++;
                    if (i == nAnswers) break;
                    if (answers[i] == entry.first) {
                        q.push({i+1, entry.second});
                    }
                }
            } else {
                for (int i = answerIndex; i < nAnswers; ++i) {
                    const auto it = node.next.find(answers[i]);
                    if (it != node.next.end()) {
                        TRIE_DEBUG("FOUND A NODE ?? ");
                        q.push({i+1, it->second});
                    }
                }
            }
            
        }
        return lb;
    }

    int getLbFromAnswerSubsetsBidirectional(const AnswersVec &answers,
        const GuessesVec &guesses,
        int answerIndex = 0,
        int nodeIndex = 0) const
    {
        std::queue<std::pair<int,int>> fq = {};
        using MyPair = std::pair<int,int>;
        auto comp = [](const MyPair p1, const MyPair p2) {
            return p1.first < p2.first; // larger indexes are evaluated first, priority_queue is backwards, smh
        };
        std::priority_queue<MyPair, std::vector<MyPair>, decltype(comp)> bq(comp);
        std::vector<int> seen(nodes.size(), 0), seenFront(nodes.size(), 0);

        TRIE_DEBUG("begin reverse serach");

        int lb = 0;
        fq.push({0, rootNodeId});
        seenFront[rootNodeId] = 1;
        for (int i = static_cast<int>(answers.size())-1; i > 0; --i) {
            if (finishNodes.count(answers[i]) == 0) continue;
            for (auto v: finishNodes.at(answers[i])) {
                seen[v] = 1;
                bq.push({i-1, v});
                TRIE_DEBUG("ADD BACK [" << (i-1) << ", " << v << "]")
            }
        }

        TRIE_DEBUG("bq size: " << bq.size());

        int nAnswers = answers.size();

        while (!(fq.empty() && bq.empty())) {
            if (bq.size() == 0 || (fq.size() > 0 && fq.size() <= bq.size())) {
                const auto [answerIndex, nodeIndex] = fq.front(); fq.pop();
                const auto &node = nodes[nodeIndex];
                lb = std::max(lb, node.lb);
                if (lb > 0) break;
                if (bq.size() && answerIndex-2 > bq.top().first) {
                    // they have crossed over!
                    TRIE_DEBUG("cross over (1)");
                    return 0;
                }
                if (answerIndex == nAnswers) continue;

                TRIE_DEBUG("FRONT [" << answerIndex << ", " << nodeIndex << "] #edges: " << node.next.size());
                for (int i = answerIndex; i < nAnswers; ++i) {
                    auto it = node.next.find(answers[i]);
                    if (it != node.next.end()) {
                        TRIE_DEBUG("FOUND A NODE [" << (i+1) << ", " << it->second << "]");
                        if (seen[it->second]) {
                            lb = 1;
                            break;
                        }
                        seenFront[it->second] = 1;
                        fq.push({i+1, it->second});
                    }
                }
            } else {
                auto [answerIndex, nodeIndex] = bq.top(); bq.pop();
                if (answerIndex == 0) continue;
                if (lb > 0) break;
                //const auto &node = nodes[nodeIndex];

                if (fq.size() && fq.front().first-2 > answerIndex) {
                    // they have crossed over!
                    TRIE_DEBUG("cross over (2)");
                    //return 0;
                }

                TRIE_DEBUG("BACK [" << answerIndex << ", " << nodeIndex << "] #edges: " << node.next.size());
                for (int i = answerIndex; i >= 0; --i) {
                    // auto it = node.prev.find(answers[i]);
                    // if (it != node.prev.end()) {
                    //     TRIE_DEBUG("FOUND A NODE [" << (i-1) << ", " << it->second << "]");
                    //     if (seen[it->second]) continue;
                    //     if (seenFront[it->second]) {
                    //         lb = 1;
                    //         break;
                    //     }
                    //     seen[it->second] = 1;
                    //     bq.push({i-1, it->second});
                    // }
                }
            }
        }
        return lb;
    }

private:
    int getNewNode() {
        nodes.push_back(BiTrieNode());
        return nodes.size()-1;
    }
};
