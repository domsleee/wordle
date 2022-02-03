#pragma once
#include <string>

std::string getPattern(const std::string &word, const std::string &answer);

struct PatternGetter {
    PatternGetter(const std::string &s): answer(s) {}

    std::string getPatternFromWord(const std::string &word) const {
        return getPattern(word, answer);
    }

  private:
    std::string answer;
};
