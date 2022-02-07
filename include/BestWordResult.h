#pragma once
#include <string>

struct BestWordResult {
    double prob;
    std::string word;
    BestWordResult() {}

    BestWordResult(double prob, const std::string &word)
        : prob(prob),
          word(word)
        {}
    
    friend inline bool operator==(const BestWordResult &a, const BestWordResult &b) = default;
};

template <>
struct std::hash<BestWordResult> {
    std::size_t operator()(const BestWordResult& k) const {
        return ((std::hash<double>()(k.prob)
                ^ (std::hash<std::string>()(k.word) << 1)) >> 1);
    }
};
