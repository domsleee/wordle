#pragma once
#include <string>

struct BestWordResult {
    double prob;
    std::string word;
    BestWordResult() {}

    BestWordResult(double prob, const std::string &word)
        : prob(prob),
          word(word){}
};
