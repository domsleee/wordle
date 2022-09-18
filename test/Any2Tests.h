#pragma once
#include "../third_party/catch.hpp"
#include "ParseArgs.h"
#include "Runner.h"
#include "TestDefs.h"


TEST_CASE("any4in2") {
    auto argHelper = ArgHelper({"solve", "-Seartolsinc", "-I0", "-g2", "--guesses", "ext/wordle-combined.txt", "--answers", "test/files/BatchCatchHatchPatch.txt"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto guesses = readFromFile(GlobalArgs.guesses);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r[0].pairs.size() == 12972, "all results");
    for (int i = 0; i < static_cast<int>(r[0].pairs.size()); ++i) {
        auto p = r[0].pairs[i];
        const auto word = guesses[p.wordIndex];
        REQUIRE_MESSAGE(r[0].pairs[i].numWrong > 0, FROM_SS("OK? " << i << ", " << word));
    }
}


TEST_CASE("any6in3") {
    auto argHelper = ArgHelper({"solve", "-Seartolsinc", "-I0", "-g3", "--guesses", "ext/wordle-combined.txt", "--answers", "test/files/6in3.txt"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto guesses = readFromFile(GlobalArgs.guesses);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r[0].pairs.size() == 12972, "all results");
    for (int i = 0; i < static_cast<int>(r[0].pairs.size()); ++i) {
        auto p = r[0].pairs[i];
        const auto word = guesses[p.wordIndex];
        REQUIRE_MESSAGE(r[0].pairs[i].numWrong > 0, FROM_SS("OK? " << i << ", " << word));
    }
}

TEST_CASE("SolverHelper") {
    REQUIRE(1 == SolverHelper::getMaxGuaranteedSolvedInRemDepth(1));
    REQUIRE(3 == SolverHelper::getMaxGuaranteedSolvedInRemDepth(2));
    int i = 1;
    REQUIRE_MESSAGE(1 == SolverHelper::getMaxGuaranteedSolvedInRemDepth(i), "int");
}
