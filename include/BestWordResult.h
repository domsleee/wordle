#pragma once
#include <string>
#include "Util.h"

struct BestWordResult {
    double probWrong;
    IndexType wordIndex;
    BestWordResult() {}

    BestWordResult(double probWrong, IndexType wordIndex)
        : probWrong(probWrong),
          wordIndex(wordIndex){}
};
