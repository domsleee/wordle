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

    auto lambda = [&]<bool isEasyMode>() -> bool {
        START_TIMER(precompute);
        auto solver = AnswersAndGuessesSolver<isEasyMode>(answers, guesses, GlobalArgs.maxTries, GlobalArgs.maxIncorrect);
        AttemptStateFast::buildForReverseIndexLookup(solver.reverseIndexLookup);
        AttemptStateToUse::buildWSLookup(solver.reverseIndexLookup);
        PatternGetterCached::buildCache(solver.reverseIndexLookup);
        END_TIMER(precompute);

        if (GlobalArgs.parallel) {
            return RunnerMulti::run(solver);
        }

        START_TIMER(total);
        std::vector<std::string> wordsToSolve = answers;//getWordsToSolve();//{getWordsToSolve()[4]};
        DEBUG("calc total.." << wordsToSolve.size());
        std::vector<long long> results(wordsToSolve.size(), 0);
        std::vector<std::string> unsolved = {};
        int correct = 0;

        bool getFirstWord = true;
        if (GlobalArgs.firstWord != "") {
            solver.startingWord = GlobalArgs.firstWord;
            getFirstWord = false;
        }

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

    if (GlobalArgs.hardMode) {
        return lambda.template operator()<false>();
        //return lambda<false>();
    }
    return lambda.template operator()<true>();
}