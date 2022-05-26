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
    std::array<int, 27*5> childByLetter = {};
    NonLetterTrieNode() {
        childByLetter.fill(-1);
    }
};

struct NonLetterLookup {
    inline static std::map<std::string, int> stringPatternToId = {};
    inline static std::unordered_map<int, std::string> idToStringPattern = {};
    inline static std::vector<NonLetterNode> nodes = {};
    inline static std::vector<NonLetterTrieNode> trieNodes = {};

    static void build() {
        START_TIMER(NonLetterLookup);
        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            getIndex(GlobalState.allGuesses[i]);
        }

        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            buildNode(GlobalState.allGuesses[i], true);
        }
        DEBUG("nonLetterLookup: numNodes: " << nodes.size());
        END_TIMER(NonLetterLookup);
    }

    static int buildNode(std::string s, bool forceCreateChildren = false) {
        auto id = getIndex(s);
        if (!id.first && !forceCreateChildren) return id.second;

        for (int i = 0; i < 5; ++i) {
            if (s[i] == '.') continue;
            auto oldSi = s[i];
            s[i] = '.';
            auto childId = buildNode(s);
            nodes[id.second].childIds.push_back(childId);
            trieNodes[id.second].childByLetter[i*27 + letterToInd(oldSi)] = childId;
            s[i] = oldSi;
        }
        return id.second;
    }

    static int letterToInd(char letter) {
        return letter == '.' ? 26 : (letter-'a');
    }

    static std::pair<bool, int> getIndex(const std::string &s) {
        auto it = stringPatternToId.find(s);
        if (it != stringPatternToId.end()) return {false, it->second};

        auto id = nodes.size();
        nodes.resize(id+1);
        trieNodes.resize(id+1);
        stringPatternToId[s] = id;;
        idToStringPattern[id] = s;
        return {true, id};
    }
};
