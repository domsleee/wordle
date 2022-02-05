#include "../include/Tests.h"
#include "../include/AttemptState.h"
#include "../include/Util.h"

#include <iostream>

void testAttemptState() {
    std::vector<std::string> words = {
        "ought",
        "duchy",
        "pithy",
        "night",
        "might",
        "eight",
        "wight",
        "tithe",
        "itchy",
        "fight",
        "niche",
        "right",
        "tight"
    };

    auto getter = PatternGetter("night");
    MinLetterType minLetterLimit = {};
    minLetterLimit['h'-'a'] = 1; // from blahs (___+_)
    auto answersState = AttemptState(getter, 0, words, minLetterLimit);

    auto wordState = answersState.guessWord("foehn");
    DEBUG("num words " << wordState.words.size());
    for (auto w: wordState.words) DEBUG(w);
    assertm(wordState.words.size() == 1, "one word");
    assertm(wordState.words[0] == "night", "correct word");
}

void testPatternGetter1() {
    auto getter = PatternGetter("aaron");
    auto pattern = getter.getPatternFromWord("opera");
    assertm(pattern == "?__??", "pattern should be right");
}

void testPatternGetter2() {
    auto getter = PatternGetter("absolutely");
    auto pattern = getter.getPatternFromWord("aboriginal");
    DEBUG("PATTERN " << pattern);
    assertm(pattern == "++?______?", "pattern should be right");
}

void runTests() {
    DEBUG("RUN TESTS");

    testAttemptState();
    testPatternGetter1();
    testPatternGetter2();

    auto pattern = getPattern(toLower("ALARM"), toLower("SHARD"));
    assertm(pattern == "__++_", "pattern should be right");

    DEBUG("all tests passed");
}