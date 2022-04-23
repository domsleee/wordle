#pragma once
#include "PatternIntHelpers.h"
struct JsonConverterPatternHelpers {
    const PatternType emojiToPatternInt(const std::string &s) {
        // ⬛️🟩🟨 to patternInt
        std::string patternStr = "";
        for (char emoji: s) patternStr += emojiToPatternChar(emoji);
        return PatternIntHelpers::calcPatternInt(patternStr);
    }

    char emojiToPatternChar(const char8_t *emoji) {
        if (emoji == u8"⬛️") return '_';
        if (emoji == u8"🟨") return '?';
        if (emoji == u8"🟩") return '+';

        DEBUG("UNKONWN EMOJI IN PATTERN STR " << emoji);
        exit(1);
    }

    const std::wstring patternIntToEmoji(const PatternType patternInt) {
        return L"??";
    }
};
