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

    static void build() {
        START_TIMER(NonLetterLookup);
        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            getOrCreateIndex(GlobalState.allGuesses[i]);
        }

        for (std::size_t i = 0; i < GlobalState.allGuesses.size(); ++i) {
            buildNode(GlobalState.allGuesses[i]);
        }
        DEBUG("nonLetterLookup: numNodes: " << nodes.size());
        END_TIMER(NonLetterLookup);
    }

    static int buildNode(std::string s) {
        auto id = getOrCreateIndex(s);
        if (nodes[id].childIds.size()) return id;
        DEBUG("id: " << id);

        for (int i = 0; i < 5; ++i) {
            if (s[i] == '.') continue;
            auto oldSi = s[i];
            s[i] = '.';
            auto childId = buildNode(s);
            nodes[id].childIds.push_back(childId);
            trieNodes[id].childByLetter[i*27 + letterToInd(oldSi)] = childId;
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
};