#pragma once
#include "BestWordResult.h"
#include "SolutionModel.h"

struct AnswersAndGuessesResult {
    int tries = 0;
    int numWrong = 0;
    BestWordResult firstGuessResult = {};
    std::shared_ptr<SolutionModel> solutionModel = std::make_shared<SolutionModel>();
};
