#include "../include/AttemptState.h"
#include "../include/Tests.h"
#include "../include/Util.h"
#include "../include/SimpleSolver.h"
#include "../include/AnswersAndGuessesSolver.h"

#include <algorithm>
#include <numeric>
#include <execution>

template<typename T>
void printSolverInformationInner(const T& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << solver.cacheHit << "/" << cacheTotal << " (" << 100.00 * solver.cacheHit / cacheTotal << "%)");
}

void printSolverInformation(const AnswersAndGuessesSolver& solver) {
    printSolverInformationInner(solver);
}
std::vector<std::string> getWordsToSolve();

int main(int argc, char *argv[]) {
    //runTests(); return 0;
    if (argc != 3) {
        std::cout << "USAGE: " << argv[0] << " <guesses> <answers>\n";
        return 1;
    }

    int correct = 0;
    auto guesses = readFromFile(std::string(argv[1]));
    auto answers = readFromFile(std::string(argv[2]));
    guesses = mergeAndSort(guesses, answers);
    guesses = answers;

    auto solver = AnswersAndGuessesSolver(answers, guesses);

    START_TIMER(precompute);
    AttemptState::precompute(guesses);
    solver.precompute();
    //solver.setupInitial4Words();

    END_TIMER(precompute);

    START_TIMER(total);
    std::vector<std::string> wordsToSolve = answers;//getWordsToSolve();//{getWordsToSolve()[4]};
    DEBUG("calc total.." << wordsToSolve.size());
    std::vector<long long> results(wordsToSolve.size(), 0);
    std::vector<std::string> unsolved = {};
    for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
        auto word = wordsToSolve[i];
        DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));

        solver.startingWord = "aahed";
        auto r = solver.solveWord(word, true);
        if (r != -1) correct++;
        else unsolved.push_back(word);
        results[i] = r == -1 ? 0 : r;
        //DEBUG("RES: " << r);
    }
    
    double avg = (double)std::reduce(results.begin(), results.end()) / correct;

    DEBUG("=============");
    DEBUG("MAX_TRIES   : " << MAX_TRIES);
    DEBUG("correct     : " << correct << "/" << wordsToSolve.size() << " (" << 100.0 * correct / wordsToSolve.size() << "%)");    
    DEBUG("average     : " << avg);
    printSolverInformation(solver);
    auto attemptStateTotal = AttemptState::cacheHit + AttemptState::cacheMiss;
    DEBUG("state cache : " << AttemptState::cacheHit << "/" << attemptStateTotal << " (" << 100.00 * AttemptState::cacheHit/attemptStateTotal << "%)");
    END_TIMER(total);

    if (unsolved.size() == 0) { DEBUG("ALL WORDS SOLVED!"); }
    else DEBUG("UNSOLVED WORDS:");
    for (auto w: unsolved) DEBUG(w);

    return 0;
}

std::vector<std::string> getWordsToSolve() {
    return readFromFile("./ext/wordsToSolve.txt");
}
