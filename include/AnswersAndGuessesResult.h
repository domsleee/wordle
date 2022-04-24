#pragma once
#include "BestWordResult.h"
#include "SolutionModel.h"

struct AnswersAndGuessesResult {
    TriesRemainingType tries = 0;
    BestWordResult firstGuessResult = {};
    std::shared_ptr<SolutionModel> solutionModel = std::make_shared<SolutionModel>();
};
