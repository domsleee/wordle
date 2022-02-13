#include "../include/PatternGetter.h"
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
