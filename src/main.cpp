#include "../include/AttemptState.h"
#include "../include/Tests.h"
#include "../include/Util.h"
#include "../include/SimpleSolver.h"
#include "../include/BranchSolver.h"
#include <algorithm>
#include <numeric>

template<typename T>
void printSolverInformation(T& solver) {

}

void printSolverInformation(const BranchSolver& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << solver.cacheHit << "/" << cacheTotal << " (" << 100.00 * solver.cacheHit / cacheTotal << "%)");
}

int main(int argc, char *argv[]) {
    //runTests(); return 0;
    if (argc != 2) {
        std::cout << "USAGE: " << argv[0] << " <file>\n";
        return 1;
    }

    int correct = 0;
    auto words = readFromFile(std::string(argv[1]));
    words = getWordsOfLength(words, 5);
    words = getFirstNWords(words, NUM_WORDS);
    //DEBUG(words[0] << '\n'); exit(1);
    //DEBUG(words.size()); exit(1);
    
    auto solver = SimpleSolver(words);
    solver.precompute();

    START_TIMER(total);
    std::vector<int> results(words.size());
    for (std::size_t i = 0; i < words.size(); ++i) {
        //DEBUG(words[i] << ": solving " << (i+1) << " / " << words.size() << " (" << 100.0*(i+1)/words.size() << "%)");
        auto word = words[i];
        auto r = solver.solveWord(word);
        if (r != -1) correct++;
        results[i] = r;
        //DEBUG("RES: " << r);
    }

    double avg = (double)std::reduce(results.begin(), results.end()) / correct;

    DEBUG("=============");
    DEBUG("MAX_TRIES   : " << MAX_TRIES);
    DEBUG("correct     : " << correct << "/" << words.size() << " (" << 100.0 * correct / words.size() << "%)");    
    DEBUG("average     : " << avg);
    printSolverInformation(solver);
    auto attemptStateTotal = AttemptState::cacheHit + AttemptState::cacheMiss;
    DEBUG("state cache : " << AttemptState::cacheHit << "/" << attemptStateTotal << " (" << 100.00 * AttemptState::cacheHit/attemptStateTotal << "%)");
    END_TIMER(total);
}

