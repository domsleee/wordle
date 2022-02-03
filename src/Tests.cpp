#include "../include/Tests.h"
#include "../include/AttemptState.h"
#include "../include/Util.h"

#include <iostream>

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

    testPatternGetter1();
    testPatternGetter2();

    auto pattern = getPattern(toLower("ALARM"), toLower("SHARD"));
    assertm(pattern == "__++_", "pattern should be right");

    DEBUG("all tests passed");
}