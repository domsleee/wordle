// https://github.com/andrew-t/gaming-wordle/blob/main/easy.json
#pragma once
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include "Util.h"

struct SolutionModel;

struct SolutionModel {
    std::string guess = "";
    std::unordered_map<std::string, std::shared_ptr<SolutionModel>> next = {};
    int guessesLeft = -1;
    int possibilitiesLeft = -1;
};
