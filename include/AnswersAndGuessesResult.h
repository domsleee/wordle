#pragma once
#include "BestWordResult.h"

struct AnswersAndGuessesResult {
    int tries = 0;
    BestWordResult firstGuessResult = {};
};
