#include "../include/PatternGetter.h"
#include "../include/PatternIntHelpers.h"
#include "../include/Util.h"

std::string getPattern(const std::string &word, const std::string &answer) {
    MinLetterType answerLetterCount = {};
    std::string pattern(word.size(), NULL_LETTER);

    for (std::size_t i = 0; i < word.size(); ++i) {
        auto c = answer[i];
        if (word[i] == c) {
            pattern[i] = '+';
        } else {
            answerLetterCount[c-'a']++;
        }
    }

    for (std::size_t i = 0; i < word.size(); ++i) {
        if (word[i] == answer[i]) continue;

        const auto letterInd = word[i]-'a';
        if (answerLetterCount[letterInd] > 0) {
            pattern[i] = '?';
            answerLetterCount[letterInd]--;
        } else {
            pattern[i] = '_';
        }
    }

    return pattern;
}

PatternType getPatternInteger(const std::string &word, const std::string &answer) {
    MinLetterType answerLetterCount = {};
    PatternType patternInt = 0;
    int mult = 1;

    for (std::size_t i = 0; i < word.size(); ++i) {
        auto c = answer[i];
        if (word[i] == c) {
            patternInt += PatternIntHelpers::charToInt('+') * mult;
        } else {
            answerLetterCount[c-'a']++;
        }
        mult *= 3;
    }

    mult = 1;
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (word[i] != answer[i]) {
            const auto letterInd = word[i]-'a';
            if (answerLetterCount[letterInd] > 0) {
                patternInt += PatternIntHelpers::charToInt('?') * mult;
                answerLetterCount[letterInd]--;
            } else {
                patternInt += PatternIntHelpers::charToInt('_') * mult;
            }
        }
        mult *= 3;
    }

    return patternInt;
}
