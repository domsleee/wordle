#pragma once
#include <string>

struct BestWordResult {
    double prob;
    int wordIndex;
    BestWordResult() {}

    BestWordResult(double prob, const int wordIndex)
        : prob(prob),
          wordIndex(wordIndex){}
};
