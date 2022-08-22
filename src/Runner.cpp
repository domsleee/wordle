#include "EndGameAnalysis/EndGameAnalysis.h"
#include "EndGameAnalysis/EndGameDatabase.h"
#include "PatternGetterCached.h"
#include "RemoveGuessesBetterGuess/NonLetterLookup.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesWithBetterGuessCache.h"
#include "Runner.h"
#include "SolveForNIdeas/SolveFor2Ideas.h"
#include "Util.h"
#include "Verifier.h"

std::vector<RunnerMultiResult> Runner::run() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    GlobalState = _GlobalState(guesses, answers);

    auto lambda = [&]<bool isEasyMode>() -> std::vector<RunnerMultiResult> {
        auto solver = AnswersAndGuessesSolver<isEasyMode>(GlobalArgs.maxTries);

        if (GlobalArgs.pauseForAttach) { DEBUG("ENTER CHAR TO CONTINUE"); char c; std::cin >> c; }

        DEBUG("initialising...");

        START_TIMER(precompute);
        PatternGetterCached::buildCache();
        NonLetterLookup::build();
        RemoveGuessesUsingNonLetterMask::buildLetterLookup();
        RemoveGuessesWithBetterGuessCache::init();
        SolveFor2Ideas::checkCanAny4BeSolvedIn2(); exit(1);
        EndGameDatabase(solver).init("list1");

        //return 0;
        END_TIMER(precompute);

        if (GlobalArgs.verify != "") {
            auto model = JsonConverter::fromFile(GlobalArgs.verify);
            auto results = Verifier::verifyModel(model, solver);
            RunnerUtil::printInfo(solver, results);
            return {};
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