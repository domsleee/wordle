#pragma once
#include "BestWordResult.h"
#include "Defs.h"

using EndGameList = std::vector<AnswersVec>;

// remDepth, e = entries for all powersets of endGame [remDepth][e]
using EndGameCache = std::vector<std::vector<std::vector<BestWordResult>>>;