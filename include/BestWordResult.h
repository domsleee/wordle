#pragma once
#include <string>
#include "Util.h"

struct BestWordResult {
    int numWrong = -1;
    IndexType wordIndex;
    BestWordResult() {}

    BestWordResult(int numWrong, IndexType wordIndex)
        : numWrong(numWrong),
          wordIndex(wordIndex){}
};
