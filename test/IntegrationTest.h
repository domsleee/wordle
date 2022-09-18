#pragma once
#include "../third_party/catch.hpp"
#include "ParseArgs.h"
#include "Runner.h"
#include "TestDefs.h"
#include "ArgHelper.h"
#include <string>
#include <vector>

TEST_CASE("G4 tenor,raced should be 17") {
    //  ./bin/solve -Seartol -p -I20 -g4 ext/wordle-guesses.txt ext/wordle-answers.txt
    auto argHelper = ArgHelper({"solve", "-Seartolsinc",  "-I20", "-g4", "--guesses", "ext/wordle-guesses.txt", "--answers", "ext/wordle-answers.txt", "--guesses-to-check", "test/files/TenorRaced.txt"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r.size() == 1, "ONE RESULT");
    const auto &pairs = r[0].pairs;
    REQUIRE_MESSAGE(pairs.size() == 3, "NUM PAIRS");
    REQUIRE_MESSAGE(pairs[0].numWrong == 17, 17);
    REQUIRE_MESSAGE(pairs[1].numWrong == 17, 17);
    REQUIRE_MESSAGE(pairs[2].numWrong == 21, 21);
}

TEST_CASE("G5 roate,soare,orate") {
    auto argHelper = ArgHelper({"solve", "-Seartolsinc", "-I0", "-g5", "--guesses", "ext/wordle-guesses.txt", "--answers", "ext/wordle-answers.txt", "--guesses-to-check", "test/files/RoateSoareOrate.txt", "--m-partitions-rem-depth", "3", "--M-partitions-rem-depth", "4"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r.size() == 1, "ONE RESULT");
    const auto &pairs = r[0].pairs;
    REQUIRE_MESSAGE(pairs.size() == 3, "NUM PAIRS");
    REQUIRE_MESSAGE(pairs[0].numWrong == 0, 0);
    REQUIRE_MESSAGE(pairs[1].numWrong == 0, 0);
    REQUIRE_MESSAGE(pairs[2].numWrong == 0, 0);
}

TEST_CASE("G6 BIGHIDDEN lanes,lears") {
    if (std::getenv("IGNORE_BIGTESTS")) {
        DEBUG("ignoring big test...");
        return;
    }
    auto argHelper = ArgHelper({"solve", "-Seartolsinc", "-I0", "-g6", "--guesses", "ext/wordle-combined.txt", "--answers", "ext/wordle-combined.txt", "--guesses-to-check", "test/files/LanesLears.txt"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r.size() == 1, "ONE RESULT");
    const auto &pairs = r[0].pairs;
    REQUIRE_MESSAGE(pairs.size() == 2, "NUM PAIRS");
    REQUIRE_MESSAGE(pairs[0].numWrong == 0, 0);
    REQUIRE_MESSAGE(pairs[1].numWrong > 0, 1);
}

TEST_CASE("edge case") {
    GlobalArgs.disableEndGameAnalysis = true;
    auto argHelper = ArgHelper({"solve", "-Seartolsinc", "-I0", "-g6", "--guesses", "ext/wordle-combined.txt", "--answers", "ext/wordle-combined.txt", "-z0", "-T"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);

    auto solver = AnswersAndGuessesSolver<true>(GlobalArgs.maxTries);
    initFromGlobalArgs();
    Runner::precompute(solver);
    std::vector<std::string> vec({"lanes", "lenes", "lines", "lunes", "lynes"});
    AnswersVec answers = {};
    for (auto s: vec) answers.push_back(GlobalState.getIndexForWord(s));
    auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
    auto numWrong = solver.minOverWordsLeastWrong(answers, guesses, 2, 0, 1);
    REQUIRE_MESSAGE(numWrong.numWrong == 0, "should find a solution");
}
