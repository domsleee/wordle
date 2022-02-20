#include "../include/GlobalArgs.h"
#include "../third_party/cxxopts.hpp"
#include "../include/Runner.h"

int main(int argc, char *argv[]) {
    cxxopts::Options options("WordleSolver", "One line description of WordleSolver");

    options.add_options()
        ("guesses", "Guesses", cxxopts::value<std::string>())
        ("answers", "Answers", cxxopts::value<std::string>())
        ("w,first-word", "First Word", cxxopts::value<std::string>()->default_value(""))
        ("m,max-tries", "Max Tries", cxxopts::value<int>()->default_value("6"))
        ("i,max-incorrect", "Max incorrect", cxxopts::value<int>()->default_value("0"))
        ("num-to-restrict", "Reduce the number of guesses", cxxopts::value<int>()->default_value("50000"))
        ("p,parallel", "Use parallel processing")
        ("r,reduce-guesses", "Reduce the number of guesses")
        ("force-sequential", "No parallel when generating cache")
        ("hard-mode", "Solve the hard mode of wordle")
        ("h,help", "Print usage")
    ;

    options.parse_positional({"guesses", "answers"});

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }
    GlobalArgs.guesses = result["guesses"].as<std::string>();
    GlobalArgs.answers = result["answers"].as<std::string>();

    //values
    GlobalArgs.numToRestrict = result["num-to-restrict"].as<int>();
    GlobalArgs.maxTries = result["max-tries"].as<int>();
    GlobalArgs.maxIncorrect = result["max-incorrect"].as<int>();
    GlobalArgs.firstWord = result["first-word"].as<std::string>();

    // flags
    GlobalArgs.parallel = result.count("parallel");
    GlobalArgs.reduceGuesses = result.count("reduce-guesses");
    GlobalArgs.forceSequential = result.count("force-sequential");
    GlobalArgs.hardMode = result.count("hard-mode");

    return Runner::run();
}
