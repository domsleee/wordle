#include "../include/Runner.h"

#include "../include/Util.h"
#include "../include/PatternGetterCached.h"
#include "../include/RunnerMulti.h"
#include "../include/RemoveGuessesBetterGuess.h"
#include "../include/GuessesRemainingAfterGuessCacheSerialiser.h"
#include "../include/Verifier.h"


int Runner::run() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    guesses = mergeAndUniq(answers, guesses);
    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    auto lambda = [&]<bool isEasyMode, bool isGetLowestAverage>() -> bool {
        auto solver = AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage>(answers, guesses, GlobalArgs.maxTries, GlobalArgs.maxIncorrect);

        START_TIMER(precompute);
        PatternGetterCached::buildCache(solver.reverseIndexLookup);
        //AttemptStateFast::buildForReverseIndexLookup(solver.reverseIndexLookup);
        GuessesRemainingAfterGuessCache::buildCache(solver.reverseIndexLookup);
        //AttemptStateFast::clearCache();
        solver.buildStaticState();
        //GuessesRemainingAfterGuessCacheSerialiser::copy();
        //GuessesRemainingAfterGuessCacheSerialiser::writeToFile("oh");
        //return 0;
        END_TIMER(precompute);

        if (GlobalArgs.verify != "") {
            auto model = JsonConverter::fromFile(GlobalArgs.verify);
            auto results = Verifier::verifyModel(model, solver);
            RunnerUtil::printInfo(solver, results);
            return 0;
        }

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
        std::vector<int64_t> results(wordsToSolve.size(), 0);
        std::vector<std::string> unsolved = {};
        int correct = 0;

        for (std::size_t i = 0; i < wordsToSolve.size(); ++i) {
            std::string word = wordsToSolve[i];
            DEBUG(word << ": solving " << getPerc(i+1, wordsToSolve.size()) << ", " << getPerc(correct, i));
            auto r = solver.solveWord(word, std::make_shared<SolutionModel>()).tries;
            if (r != -1) correct++;
            else unsolved.push_back(word);
            results[i] = r == TRIES_FAILED ? 0 : r;
            //DEBUG("RES: " << r);
        }
        
        RunnerUtil::printInfo(solver, results);    
        END_TIMER(total);

        if (unsolved.size() == 0) { DEBUG("ALL WORDS SOLVED!"); }
        else DEBUG("UNSOLVED WORDS: " << unsolved.size());
        //for (auto w: unsolved) DEBUG(w);

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