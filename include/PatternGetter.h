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

    static inline int charToInt(char c) {
        switch(c) {
            case '?': return 0;
            case '_': return 1;
            case '+': return 2;
            default: {
                DEBUG("UNKONWN CHAR IN PATTERN STR " << c);
                exit(1);
            }
        }
    }

  private:
    std::string answer;
};
