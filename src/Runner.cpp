#include "../include/Runner.h"

#include "../include/Util.h"
#include "../include/AttemptStateFast.h"
#include "../include/PatternGetterCached.h"
#include "../include/RunnerMulti.h"

int Runner::run() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    guesses = mergeAndUniq(answers, guesses);
    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    auto lambda = [&]<bool isEasyMode, bool isGetLowestAverage>() -> bool {
        START_TIMER(precompute);
        auto solver = AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage>(answers, guesses, GlobalArgs.maxTries, GlobalArgs.maxIncorrect);
        AttemptStateFast::buildForReverseIndexLookup(solver.reverseIndexLookup);
        AttemptStateToUse::buildWSLookup(solver.reverseIndexLookup);
        AttemptStateFast::clearCache();
        PatternGetterCached::buildCache(solver.reverseIndexLookup);
        solver.buildStaticState();
        END_TIMER(precompute);

        if (GlobalArgs.firstWord != "") {
            solver.startingWord = GlobalArgs.firstWord;
        }

        if (GlobalArgs.parallel) {
            return RunnerMulti::run(solver);
        }

        START_TIMER(total);
        auto indexesToCheck = RunnerMulti::getIndexesToCheck(answers);
        std::vector<std::string> wordsToSolve = {};
        for (auto ind: indexesToCheck) wordsToSolve.push_back(answers[ind]);

        DEBUG("calc total.." << wordsToSolve.size());
        std::vector<long long> results(wordsToSolve.size(), 0);
        std::vector<std::string> unsolved = {};
        int correct = 0;

        bool getFirstWord = GlobalArgs.firstWord == "";

        for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
            std::string word = wordsToSolve[i];
            DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));
            auto r = solver.solveWord(word, getFirstWord);
            if (r != -1) correct++;
            else unsolved.push_back(word);
            results[i] = r == -1 ? 0 : r;
            //DEBUG("RES: " << r);
        }
        
        RunnerUtil::printInfo(solver, results);    
        END_TIMER(total);

        if (unsolved.size() == 0) { DEBUG("ALL WORDS SOLVED!"); }
        else DEBUG("UNSOLVED WORDS: " << unsolved.size());
        //for (auto w: unsolved) DEBUG(w);
        DEBUG("guess word ct " << AttemptStateFast::guessWordCt);

        return 0;
    };

    auto lambda2 = [&]<bool isEasyMode>() -> bool {
        if (GlobalArgs.isGetLowestAverage) {
            return lambda.template operator()<isEasyMode, true>();
        }
        return lambda.template operator()<isEasyMode, false>();
    };

    if (GlobalArgs.hardMode) {
        return lambda2.template operator()<false>();
        //return lambda<false>();
    }
    return lambda2.template operator()<true>();
}