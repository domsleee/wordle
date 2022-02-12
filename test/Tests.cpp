#include "../include/Tests.h"
#include "../include/AttemptState.h"
#include "../include/Util.h"
#define CATCH_CONFIG_MAIN
#include "../third_party/catch.hpp"

#include <iostream>

#define REQUIRE_MESSAGE(cond, msg) do { INFO(msg); REQUIRE(cond); } while((void)0, 0)

TEST_CASE("attempt state") {
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
    auto answersState = AttemptState(getter, {});

    auto wordState = answersState.guessWord("foehn", words);
    INFO("num words " << wordState.words.size());
    for (auto w: wordState.words) INFO(w);
    REQUIRE_MESSAGE(wordState.words.size() == 1, "one word");
    REQUIRE_MESSAGE(wordState.words[0] == "night", "correct word");
}

TEST_CASE("PatternGetter 1") {
    auto getter = PatternGetter("aaron");
    auto pattern = getter.getPatternFromWord("opera");
    REQUIRE_MESSAGE(pattern == "?__??", "pattern should be right");
}

TEST_CASE("PatternGetter 2") {
    auto getter = PatternGetter("absolutely");
    auto pattern = getter.getPatternFromWord("aboriginal");
    INFO("PATTERN " << pattern);
    REQUIRE_MESSAGE(pattern == "++?______?", "pattern should be right");
}

TEST_CASE("PatternGetter 3") {
    auto pattern = getPattern(toLower("ALARM"), toLower("SHARD"));
    REQUIRE_MESSAGE(pattern == "__++_", "pattern should be right");
}

TEST_CASE
