#include "../include/Runner.h"

#include "../include/Util.h"
#include "../include/PatternGetterCached.h"
#include "../include/RunnerMulti.h"
#include "../include/Verifier.h"
#include "../include/EndGameAnalysis.h"
#include "../include/SolveFor2Ideas.h"
#include "../include/RemoveGuessesWithBetterGuessCache.h"
#include "../include/NonLetterLookup.h"
#include "../include/EndGameDatabase.h"

int Runner::run() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    GlobalState = _GlobalState(guesses, answers);

    auto lambda = [&]<bool isEasyMode>() -> bool {
        auto solver = AnswersAndGuessesSolver<isEasyMode>(GlobalArgs.maxTries);

        if (GlobalArgs.pauseForAttach) { DEBUG("ENTER CHAR TO CONTINUE"); char c; std::cin >> c; }

        DEBUG("initialising...");

        START_TIMER(precompute);
        PatternGetterCached::buildCache();
        NonLetterLookup::build();
        RemoveGuessesWithNoLetterInAnswers::buildLetterLookup();
        RemoveGuessesWithBetterGuessCache::init();
        //SolveFor2Ideas::checkCanAny4BeSolvedIn2(); exit(1);
        EndGameDatabase(solver).init("list1");

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

        return GlobalArgs.parallel
            ? RunnerMulti<true>().run(solver)
            : RunnerMulti<false>().run(solver);
    };

    return GlobalArgs.hardMode
        ? lambda.template operator()<false>()
        : lambda.template operator()<true>();

}