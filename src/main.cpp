#include "../include/AttemptState.h"
#include "../include/Tests.h"
#include "../include/Util.h"
#include "../include/SimpleSolver.h"
#include "../include/BranchSolver.h"
#include "../include/AnswersAndGuessesSolver.h"

#include <algorithm>
#include <numeric>
#include <execution>
int simpleSolverInv(const std::vector<std::string> &guesses, const std::vector<std::string> &answers);

template<typename T>
void printSolverInformationInner(const T& solver) {
    auto cacheTotal = solver.cacheHit + solver.cacheMiss;
    DEBUG("solver cache: " << solver.cacheHit << "/" << cacheTotal << " (" << 100.00 * solver.cacheHit / cacheTotal << "%)");
}

void printSolverInformation(const BranchSolver& solver) {
    printSolverInformationInner(solver);
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

    //return simpleSolverInv(guesses, answers);

    //words = getWordsOfLength(words, 5);
    answers = getFirstNWords(answers, NUM_WORDS);
    //DEBUG(words[0] << '\n'); exit(1);
    //DEBUG(words.size()); exit(1);
    
    auto solver = AnswersAndGuessesSolver(answers, answers);
    solver.precompute();
    //solver.setupInitial4Words();

    START_TIMER(total);

    auto wordsToSolve = getWordsToSolve();
    DEBUG("calc total.." << wordsToSolve.size());
    std::vector<long long> results(wordsToSolve.size(), 0);
    std::vector<std::string> unsolved = {};
    for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
        auto word = wordsToSolve[i];
        DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));

        auto r = solver.solveWord(word);
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

    DEBUG("UNSOLVED");
    for (auto w: unsolved) DEBUG(w);

    return 0;
}

int simpleSolverInv(const std::vector<std::string> &guesses, const std::vector<std::string> &answers) {
    auto solver = AnswersAndGuessesSolver(answers, guesses);
    solver.precompute();
    std::vector<std::pair<int, int>> numCorrect(answers.size(), {0, 0});
    std::pair<int,int> best = {0, 0};
    
    for (std::size_t i = 0; i < answers.size(); ++i) {
        DEBUG("doing: " << getPerc(i, answers.size()));
        //solver.firstWord = answers[i];
        numCorrect[i].second = i;
        auto res = std::transform_reduce(
            std::execution::par,
            answers.begin(), answers.end(),
            0,
            [](int a, int b) { return a + b; },
            [&](const std::string &a) {
                auto r = solver.solveWord(a);
                return r != -1 ? 1 : 0;
            }
        );

        numCorrect[i].first = res;

        DEBUG("res: " << getPerc(numCorrect[i].first, answers.size()) << ", best: " << getPerc(best.first, answers.size()));
        if (numCorrect[i].first > best.first) {
            best = numCorrect[i];
        }
        //DEBUG("RES: " << r);
    }

    std::sort(numCorrect.begin(), numCorrect.end(), std::greater<std::pair<int,int>>());
    DEBUG("best words");
    for (int i = 0; i < 5; ++i) {
        DEBUG(answers[i] << ": " << getPerc(numCorrect[i].first, answers.size()));
    }

    exit(0);
}

std::vector<std::string> getWordsToSolve() {
    return readFromFile("./ext/wordsToSolve.txt");
}
