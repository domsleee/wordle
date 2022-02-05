#include "../include/PatternGetter.h"
#include "../include/Util.h"

std::string getPattern(const std::string &word, const std::string &answer) {
    int answerLetterCount[26] = {};
    for (int i = 0; i < 26; ++i) answerLetterCount[i] = 0;
    for (auto c: answer) {
        answerLetterCount[c-'a']++;
    }

    for (std::size_t i = 0; i < answer.size(); ++i) {
        if (word[i] == answer[i]) {
            auto c = word[i];
            answerLetterCount[c-'a']--;
        }
    }

    std::string pattern(word.size(), NULL_LETTER);
    for (std::size_t i = 0; i < word.size(); ++i) {
        if (word[i] == answer[i]) {
            pattern[i] = '+';
            continue;
        }

        auto letterInd = word[i]-'a';
        if (answerLetterCount[letterInd] > 0) {
            pattern[i] = '?';
            answerLetterCount[letterInd]--;
        } else {
            pattern[i] = '_';
        }
    }

    return pattern;
}
