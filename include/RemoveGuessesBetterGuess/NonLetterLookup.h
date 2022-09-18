#pragma once
#include <vector>
#include <map>
#include <string>
#include "GlobalState.h"
#include "Util.h"

struct NonLetterNode {
    std::vector<int> childIds = {};
};

struct NonLetterTrieNode {
    std::array<int, 5> childByPosition = {};
    NonLetterTrieNode() {
        childByPosition.fill(-1);
    }
};

struct CompressedNode {
    std::vector<int> childIds = {};
    CompressedNode() {
    }
};

struct CompressedTrieNode {
    std::array<int, 27> childByLetter = {};
    CompressedTrieNode() {
        childByLetter.fill(-1);
    }
};

namespace NonLetterLookupHelpers {
    static int letterToInd(char letter) {
        return letter == '.' ? 26 : (letter-'a');
    }
};
using namespace NonLetterLookupHelpers;


struct NonLetterLookup {
    inline static std::map<std::string, int> stringPatternToId = {};
    inline static std::unordered_map<int, std::string> idToStringPattern = {};
    inline static std::vector<NonLetterNode> nodes = {};
    inline static std::vector<NonLetterTrieNode> trieNodes = {};
    inline static std::vector<CompressedNode> compressedNodes = {};
    inline static std::vector<CompressedTrieNode> compressedTrieNodes = {};
    inline static std::vector<bool> compressedBuilt = {};
    inline static std::vector<std::vector<uint8_t>> guessIndexToCharIndexes = {};

    static void build() {
        START_TIMER(NonLetterLookup);
        resetState();
        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            getOrCreateIndex(GlobalState.allGuesses[i]);
        }

        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            buildNode(GlobalState.allGuesses[i]);
        }
        DEBUG("nonLetterLookup: numNodes: " << nodes.size());
        END_TIMER(NonLetterLookup);

        guessIndexToCharIndexes.resize(GlobalState.allGuesses.size());
        for (IndexType guessIndex = 0; guessIndex < GlobalState.allGuesses.size(); ++guessIndex) {
            auto &charVec = guessIndexToCharIndexes[guessIndex];
            for (int i = 0; i < 5; ++i) {
                char c = GlobalState.allGuesses[guessIndex][i];
                if (std::find(charVec.begin(), charVec.end(), letterToInd(c)) == charVec.end()) {
                    charVec.push_back(letterToInd(c));
                }
            }
        }

        buildCompressed();
    }

    static void resetState() {
        stringPatternToId = {};
        idToStringPattern = {};
        nodes = {};
        trieNodes = {};
        compressedNodes = {};
        compressedTrieNodes = {};
        compressedBuilt = {};
        guessIndexToCharIndexes = {};
    }

    static int buildNode(std::string s) {
        auto id = getOrCreateIndex(s);
        if (nodes[id].childIds.size()) return id;
        for (int i = 0; i < 5; ++i) {
            if (s[i] == '.') continue;
            auto oldSi = s[i];
            s[i] = '.';
            auto childId = buildNode(s);
            nodes[id].childIds.push_back(childId);
            trieNodes[id].childByPosition[i] = childId;
            s[i] = oldSi;
        }
        return id;
    }

    static int getOrCreateIndex(const std::string &s) {
        auto it = stringPatternToId.find(s);
        if (it != stringPatternToId.end()) return it->second;

        auto id = nodes.size();
        nodes.resize(id+1);
        trieNodes.resize(id+1);
        stringPatternToId[s] = id;
        idToStringPattern[id] = s;
        return id;
    }

    static void buildCompressed() {
        compressedBuilt.assign(nodes.size(), false);
        compressedNodes.resize(nodes.size());
        compressedTrieNodes.resize(nodes.size());
        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            buildCompressedNode(GlobalState.allGuesses[i]);
        }
        int ct = 0, compressedEdges = 0, edges = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (compressedBuilt[i]) {
                ct++;
                compressedEdges += compressedNodes[i].childIds.size();
            }
            edges += nodes[i].childIds.size();
        }
        DEBUG("#nodes: " << nodes.size() << ", edges: " << edges << ", #compressedNodes: " << ct << ", compressedEdges: " << compressedEdges);
    }

    static int buildCompressedNode(std::string s) {
        auto id = getOrCreateIndex(s);
        if (compressedBuilt[id]) return id;
        compressedBuilt[id] = true;

        int8_t seen[27] = {0};
        for (char c: s) {
            int ind = letterToInd(c);
            if (c == '.' || seen[ind]) continue;
            seen[ind] = 1;
            std::string newS = s;
            for (int i = 0; i < 5; ++i) if (newS[i] == c) newS[i] = '.';
            auto childId = buildCompressedNode(newS);
            compressedNodes[id].childIds.push_back(childId);
            compressedTrieNodes[id].childByLetter[letterToInd(c)] = childId;
        }
        return id;
    }

};