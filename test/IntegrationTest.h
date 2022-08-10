#pragma once
#include "../third_party/catch.hpp"
#include "ParseArgs.h"
#include "Runner.h"
#include "TestDefs.h"


struct ArgHelper {
    ArgHelper(std::vector<std::string> args)
      : args(args),
        argv(getArgv()) {}

    std::pair<int, char**> getArgcArgv() { return {args.size(), argv}; };
    
    ~ArgHelper() {
        for (std::size_t i = 0; i < args.size(); ++i) free(argv[i]);
        free(argv);
    }
private:
    const std::vector<std::string> args;
    char** argv;

    char **getArgv() {
        char **myArgv = (char **)malloc(sizeof(char*) * args.size());
        for (std::size_t i = 0; i < args.size(); ++i) {
            myArgv[i] = strdup(args[i].c_str());
        }
        return myArgv;
    }
};


TEST_CASE("tenor,raced should be 17") {
    //  ./bin/solve -Seartol -p -I20 -g4 ext/wordle-guesses.txt ext/wordle-answers.txt
    auto argHelper = ArgHelper({"solve", "-Seartolsinc",  "-I20", "-g4", "--guesses", "ext/wordle-guesses.txt", "--answers", "ext/wordle-answers.txt", "--guesses-to-check", "test/TenorRaced.txt"});
    auto [argc, argv] = argHelper.getArgcArgv();
    parseArgs(argc, argv);
    auto r = Runner::run();
    REQUIRE_MESSAGE(r.size() == 1, "ONE RESULT");
    const auto &pairs = r[0].pairs;
    REQUIRE_MESSAGE(pairs.size() == 3, "NUM PAIRS");
    REQUIRE_MESSAGE(pairs[0].numWrong, 17);
    REQUIRE_MESSAGE(pairs[1].numWrong, 17);
    REQUIRE_MESSAGE(pairs[2].numWrong, 21);

}