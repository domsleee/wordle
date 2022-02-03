#include "../include/AttemptState.h"
#include "../include/Tests.h"
#include "../include/Util.h"
#include "../include/SimpleSolver.h"
#include "../include/BranchSolver.h"

int main(int argc, char *argv[]) {
    //runTests(); return 0;
    if (argc != 2) {
        std::cout << "USAGE: " << argv[0] << " <file>\n";
        return 1;
    }

    int failed = 0;
    auto words = readFromFile(std::string(argv[1]));
    words = getWordsOfLength(words, 5);
    words = getFirstNWords(words, 350);
    //DEBUG(words[0] << '\n'); exit(1);
    //DEBUG(words.size()); exit(1);
    
    auto solver = BranchSolver(words);
    solver.precompute();

    START_TIMER(total);
    for (std::size_t i = 0; i < words.size(); ++i) {
        DEBUG(words[i] << ": solving " << (i+1) << " / " << words.size() << " (" << 100.0*(i+1)/words.size() << "%)");
        auto word = words[i];
        auto r = solver.solveWord(word);
        if (r == -1) failed++;
        DEBUG("RES: " << r);
    }

    DEBUG("faileD: " << failed << " of " << words.size());
    DEBUG("cache size: " << solver.cacheSize << ", cache hit: " << solver.cacheHit << ", cache miss: " << solver.cacheMiss);
    DEBUG("cache hit % " << 100.00 * solver.cacheHit / (solver.cacheMiss + solver.cacheHit) << "%");
    END_TIMER(total);


}