#include "../include/AttemptState.h"
#include "../include/AttemptStateFast.h"
#include "../include/AttemptStateFaster.h"

#include "../third_party/cxxopts.hpp"

#include "../include/Util.h"
#include "../include/SimpleSolver.h"
#include "../include/AnswersAndGuessesSolver.h"
#include "../include/RunnerUtil.h"
#include "../include/MultiRunner.h"

#include <algorithm>
#include <numeric>
#include <execution>

std::vector<std::string> getWordsToSolve();

int main(int argc, char *argv[]) {
    //runTests(); return 0;
    cxxopts::Options options("WordleSolver", "One line description of WordleSolver");
    
    options.add_options()
        ("m,max-tries", "Max Tries", cxxopts::value<int>()->default_value("6"))
        ("i,max-incorrect", "Max incorrect", cxxopts::value<int>()->default_value("0"))
        ("p,parallel", "Use parallel processing")
        ("r,reduce-guesses", "Reduce the number of guesses")
        ("guesses", "Guesses", cxxopts::value<std::string>())
        ("answers", "Answers", cxxopts::value<std::string>())
        ("h,help", "Print usage")
    ;

    options.parse_positional({"guesses", "answers"});

    auto result = options.parse(argc, argv);
    auto maxTries = result["max-tries"].as<int>();
    auto maxIncorrect = result["max-incorrect"].as<int>();

    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

    auto guesses = readFromFile(result["guesses"].as<std::string>());
    auto answers = readFromFile(result["answers"].as<std::string>());
    guesses = mergeAndUniq(answers, guesses);
    //answers = getFirstNWords(answers, 2);
    if (result.count("reduce-guesses")) {
        guesses = answers;
    }

    START_TIMER(precompute);
    auto solver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses, maxTries, maxIncorrect);
    AttemptStateFast::buildForReverseIndexLookup(solver.reverseIndexLookup);
    AttemptStateToUse::buildWSLookup(solver.reverseIndexLookup);
    END_TIMER(precompute);

    if (result.count("parallel")) {
        return MultiRunner::run(solver);
    }

    START_TIMER(total);
    std::vector<std::string> wordsToSolve = {"awake"};// answers;//getWordsToSolve();//{getWordsToSolve()[4]};
    DEBUG("calc total.." << wordsToSolve.size());
    std::vector<long long> results(wordsToSolve.size(), 0);
    std::vector<std::string> unsolved = {};
    int correct = 0;

    for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
        std::string word = wordsToSolve[i];
        DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));

        solver.startingWord = "least"; // pleat solved 326/2315 (14.08%)
        auto r = solver.solveWord(word, false);
        if (r != -1) correct++;
        else unsolved.push_back(word);
        if (EARLY_EXIT && r == -1) break;
        results[i] = r == -1 ? 0 : r;
        //DEBUG("RES: " << r);
        break;
    }
    

    RunnerUtil::printInfo(solver, results);    
    END_TIMER(total);

    if (unsolved.size() == 0) { DEBUG("ALL WORDS SOLVED!"); }
    else DEBUG("UNSOLVED WORDS: " << unsolved.size());
    for (auto w: unsolved) DEBUG(w);
    DEBUG("guess word ct " << AttemptStateFast::guessWordCt);


    return 0;
}

std::vector<std::string> getWordsToSolve() {
    return readFromFile("./ext/wordsToSolve.txt");
}
