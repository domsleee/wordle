#pragma once
#include <string>

struct BestWordResult {
    double prob;
    std::string word;
    int lowerBound;
    BestWordResult() {}

    BestWordResult(double prob, const std::string &word, int lowerBound)
        : prob(prob),
          word(word),
          lowerBound(lowerBound)
        {}
    
    friend inline bool operator==(const BestWordResult &a, const BestWordResult &b) {
        return a.prob == b.prob && a.word == b.word && a.lowerBound == b.lowerBound;
    }
};

template <>
struct std::hash<BestWordResult> {
    std::size_t operator()(const BestWordResult& k) const {
        return ((std::hash<double>()(k.prob)
                ^ (std::hash<std::string>()(k.word) << 1)) >> 1)
                ^ (std::hash<int>()(k.lowerBound) << 1);
    }
};
