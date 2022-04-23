// https://github.com/andrew-t/gaming-wordle/blob/main/easy.json
#pragma once
#include <map>
#include <string>
#include "Util.h"

struct SolutionModel;

struct SolutionModel {
    std::string guess = "";
    std::map<PatternType, SolutionModel> next = {};
    int guessesLeft = -1;
    int possibilitiesLeft = -1;
};
