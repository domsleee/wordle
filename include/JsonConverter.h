#pragma once
#include "SolutionModel.h"

namespace JsonConverter {
    SolutionModel fromFile(const std::string &file);
    void toFile(const std::string &file);
};
