#pragma once
#include <string>
#include "Util.h"

struct BestWordResult {
    int probWrong;
    IndexType wordIndex;
    BestWordResult() {}

    BestWordResult(int probWrong, IndexType wordIndex)
        : probWrong(probWrong),
          wordIndex(wordIndex){}
};
