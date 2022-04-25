#include "../include/AttemptState.h"
#include "../include/AttemptStateFast.h"
#include "../include/UnorderedVector.h"

#include "../include/Util.h"
#define CATCH_CONFIG_MAIN
#include "../third_party/catch.hpp"

#include <iostream>

#define REQUIRE_MESSAGE(cond, msg) do { INFO(msg); REQUIRE(cond); } while((void)0, 0)

TEST_CASE("attempt state") {
    std::vector<std::string> startWords = {
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
    auto answersState = AttemptState(getter);
    auto vec = getVector(startWords.size(), 0);
    startWords.push_back("foehn");

    auto words = answersState.guessWord((IndexType)startWords.size()-1, vec, startWords);
    INFO("num words " << words.size());
    for (auto w: words) INFO(w);
    REQUIRE_MESSAGE(words.size() == 1, "one word");
    REQUIRE_MESSAGE(startWords[words[0]] == "night", "correct word");
}

TEST_CASE("attempt state FAST") {
    std::vector<std::string> startWords = {
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
    auto answersState = AttemptStateFast(getter);
    auto vec = getVector(startWords.size(), 0);
    startWords.push_back("foehn");
    AttemptStateFast::buildForReverseIndexLookup(startWords);

    auto words = answersState.guessWord((IndexType)startWords.size()-1, vec, startWords);
    INFO("num words " << words.size());
    for (auto w: words) INFO(w);
    REQUIRE_MESSAGE(words.size() == 1, "one word");
    REQUIRE_MESSAGE(startWords[words[0]] == "night", "correct word");
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

TEST_CASE("PatternGetter 4") {
    auto pattern = getPattern(toLower("THEED"), toLower("FILET"));
    REQUIRE_MESSAGE(pattern == "?__+_", "pattern should be right");
}


TEST_CASE("UnorderedVector basic") {
    auto vec = UnorderedVector<int>(5);
    vec[0] = 0;
    vec[1] = 1;
    vec[2] = 2;
    vec[3] = 3;
    vec[4] = 4;

    vec.deleteIndex(2);
    REQUIRE_MESSAGE(vec[2] == 4, "back should have been moved");
    REQUIRE_MESSAGE(vec.size() == 4, "should have decreased");
    vec.deleteIndex(0);

    vec.restoreValues(2);
    REQUIRE_MESSAGE(vec.size() == 5, "should have decreased");
    REQUIRE(vec[0] == 0);
    REQUIRE(vec[1] == 1);
    REQUIRE(vec[2] == 2);
    REQUIRE(vec[3] == 3);
    REQUIRE(vec[4] == 4);
}

