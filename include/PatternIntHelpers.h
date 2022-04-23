#pragma once
#include <string>
#include "Util.h"

struct PatternIntHelpers {
     static PatternType calcPatternInt(const std::string &patternStr) {
        int res = 0, mult = 1;
        for (char c: patternStr) {
            int v = charToInt(c);
            res += mult * v;
            mult *= 3;
        }
        return res;
    }

    static int charToInt(char c) {
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
};