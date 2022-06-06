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
                DEBUG("UNKNOWN CHAR IN PATTERN STR " << "'" << c << "'" << ", int: " << (int)c);
                exit(1);
            }
        }
    }

    static char intToChar(int v) {
        switch(v) {
            case 0: return '?';
            case 1: return '_';
            case 2: return '+';
            default: {
                DEBUG("UNKNOWN INT " << v);
                exit(1);
            }
        }
    }

    static std::string patternIntToString(PatternType patternInt) {
        std::string s(".....");
        for (int i = 0; i < WORD_LENGTH; ++i) {
            s[i] = intToChar(patternInt % 3);
            patternInt /= 3;
        }
        return s;
    }
};