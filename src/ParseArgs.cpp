#include "ParseArgs.h"
#include "GlobalArgs.h"
#include "../third_party/cxxopts.hpp"

void parseArgs(int argc, char *argv[]) {
    cxxopts::Options options("WordleSolver", "A solver for the wordle game.");

    options.add_options()
        ("guesses", "Guesses", cxxopts::value<std::string>()->default_value("ext/wordle-combined.txt"))
        ("answers", "Answers", cxxopts::value<std::string>()->default_value("ext/wordle-combined.txt"))

        // flags
        ("H,hard-mode", "Solve the hard mode of wordle")
        ("T,timings", "Use timings (will slow it down)")
        ("l,lowest-average", "Find a strategy for the lowest average score (default is least wrong)")
        ("p,parallel", "Use parallel processing")
        ("r,reduce-guesses", "Use the answer dictionary as the guess dictionary")
        ("s,force-sequential", "No parallel when generating cache (for profiling)")
        ("P,use-partitions", "Use partitions")

        // strings
        ("w,first-guess", "First guess", cxxopts::value<std::string>()->default_value(""))
        ("v,verify", "Solution model to verify", cxxopts::value<std::string>()->default_value(""))
        ("guesses-to-skip", "Filename of words to skip", cxxopts::value<std::string>()->default_value(""))
        ("guesses-to-check", "Filename of guesses to check", cxxopts::value<std::string>()->default_value(""))
        ("S,special-letters", "Special letters", cxxopts::value<std::string>()->default_value("eartolsincuyhpdmgbk"))
        ("o,output-result", "Output result", cxxopts::value<std::string>()->default_value("models/out.res"))

        // ints
        ("n,guess-limit-per-node", "Guess limit per node", cxxopts::value<int>()->default_value("1000000"))
        ("N,top-level-guesses", "Num top level guesses", cxxopts::value<int>()->default_value("1000000"))
        ("k,top-level-skip", "Initial offset (see N)", cxxopts::value<int>()->default_value("0"))
        ("g,max-tries", "Max tries", cxxopts::value<int>()->default_value("6"))
        ("I,max-numWrong", "Max numWrong for least wrong strategy", cxxopts::value<int>()->default_value("0"))
        ("G,max-total-guesses", "Max total guesses for lowest average strategy", cxxopts::value<int>()->default_value("500000"))
        ("num-to-restrict", "Reduce the number of first guesses", cxxopts::value<int>()->default_value("50000"))
        ("min-lb-cache", "RemDepth for storing lb cache", cxxopts::value<int>()->default_value("2"))
        ("W,workers", "Num workers (see also mem-limit)", cxxopts::value<int>()->default_value("24"))
        ("z,print-length", "Print length", cxxopts::value<int>()->default_value("-1"))

        // floats
        ("L,mem-limit", "Mem limit per thread", cxxopts::value<double>()->default_value("2"))

        ("h,help", "Print usage")
    ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    GlobalArgs.guesses = result["guesses"].as<std::string>();
    GlobalArgs.answers = result["answers"].as<std::string>();

    //values
    GlobalArgs.guessLimitPerNode = result["guess-limit-per-node"].as<int>();
    GlobalArgs.topLevelGuesses = result["top-level-guesses"].as<int>();
    GlobalArgs.topLevelSkip = result["top-level-skip"].as<int>();
    GlobalArgs.numToRestrict = result["num-to-restrict"].as<int>();
    GlobalArgs.maxTries = result["max-tries"].as<int>();
    GlobalArgs.maxWrong = result["max-numWrong"].as<int>();
    GlobalArgs.memLimitPerThread = result["mem-limit"].as<double>();
    GlobalArgs.minLbCache = result["min-lb-cache"].as<int>();
    GlobalArgs.workers = result["workers"].as<int>();
    GlobalArgs.printLength = result["print-length"].as<int>();
    GlobalArgs.firstWord = result["first-guess"].as<std::string>();
    GlobalArgs.verify = result["verify"].as<std::string>();
    GlobalArgs.outputRes = result["output-result"].as<std::string>();
    GlobalArgs.guessesToSkip = result["guesses-to-skip"].as<std::string>();
    GlobalArgs.guessesToCheck = result["guesses-to-check"].as<std::string>();
    GlobalArgs.specialLetters = result["special-letters"].as<std::string>();

    // flags
    GlobalArgs.parallel = result.count("parallel");
    GlobalArgs.reduceGuesses = result.count("reduce-guesses");
    GlobalArgs.forceSequential = result.count("force-sequential");
    GlobalArgs.hardMode = result.count("hard-mode");
    GlobalArgs.isGetLowestAverage = result.count("lowest-average");
    GlobalArgs.usePartitions = result.count("use-partitions");
    GlobalArgs.timings = result.count("timings");
} // bottom window = use-partitions