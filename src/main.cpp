#include "../include/AttemptState.h"
#include "../include/AttemptStateFast.h"

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
    if (argc != 3) {
        std::cout << "USAGE: " << argv[0] << " <guesses> <answers>\n";
        return 1;
    }

    auto guesses = readFromFile(std::string(argv[1]));
    auto answers = readFromFile(std::string(argv[2]));
    guesses = mergeAndSort(guesses, answers);
    //answers = getFirstNWords(answers, 2);
    //guesses = answers;

    MultiRunner::run(answers, guesses);

    START_TIMER(precompute);
    auto solver = AnswersAndGuessesSolver<IS_EASY_MODE>(answers, guesses);
    AttemptStateFast::buildForReverseIndexLookup(solver.reverseIndexLookup);
    END_TIMER(precompute);

    START_TIMER(total);
    std::vector<std::string> wordsToSolve = answers;//getWordsToSolve();//{getWordsToSolve()[4]};
    DEBUG("calc total.." << wordsToSolve.size());
    std::vector<long long> results(wordsToSolve.size(), 0);
    std::vector<std::string> unsolved = {};
    int correct = 0;

    for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
        auto word = wordsToSolve[i];
        DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));

        solver.startingWord = "began"; // pleat solved 326/2315 (14.08%)
        auto r = solver.solveWord(word, true);
        if (r != -1) correct++;
        else unsolved.push_back(word);
        if (EARLY_EXIT && r == -1) break;
        results[i] = r == -1 ? 0 : r;
        //DEBUG("RES: " << r);
    }
    

    RunnerUtil::printInfo(solver, results, guesses, wordsToSolve);    
    END_TIMER(total);

    if (unsolved.size() == 0) { DEBUG("ALL WORDS SOLVED!"); }
    else DEBUG("UNSOLVED WORDS: " << unsolved.size());
    //for (auto w: unsolved) DEBUG(w);
    DEBUG("guess word ct " << AttemptStateFast::guessWordCt);


    return 0;
}

std::vector<std::string> getWordsToSolve() {
    return readFromFile("./ext/wordsToSolve.txt");
}
