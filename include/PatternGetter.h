#pragma once
#include <string>
#include "Util.h"

std::string getPattern(const std::string &word, const std::string &answer);
PatternType getPatternInteger(const std::string &word, const std::string &answer);

struct PatternGetter {
    PatternGetter(const std::string &s): answer(s) {}

    std::string getPatternFromWord(const std::string &word) const {
        return getPattern(word, answer);
    }

    PatternType getPatternInt(const std::string &word) const {
        return getPatternInteger(word, answer);
    }

  private:
    std::string answer;
};
