#include "EndGameAnalysis/EndGameAnalysis.h"
#include "EndGameAnalysis/EndGameDatabase.h"
#include "PatternGetterCached.h"
#include "RemoveGuessesBetterGuess/NonLetterLookup.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesWithBetterGuessCache.h"
#include "Runner.h"
#include "GlobalState.h"
#include "SolveForNIdeas/SolveForNIdeas.h"
#include "Util.h"
#include "Verifier.h"

std::vector<RunnerMultiResult> Runner::run() {
    initFromGlobalArgs();

    auto lambda = [&]<bool isEasyMode>() -> std::vector<RunnerMultiResult> {
        auto solver = AnswersAndGuessesSolver<isEasyMode>(GlobalArgs.maxTries);

        if (GlobalArgs.pauseForAttach) { DEBUG("ENTER CHAR TO CONTINUE"); char c; std::cin >> c; }

        Runner::precompute(solver);

        if (GlobalArgs.verify != "") {
            auto model = JsonConverter::fromFile(GlobalArgs.verify);
            auto results = Verifier::verifyModel(model, solver);
            RunnerUtil::printInfo(solver, results);
            return {};
        }

        if (GlobalArgs.firstWord != "") {
            // solver.startingWord = GlobalArgs.firstWord;
        }

        return GlobalArgs.parallel
            ? RunnerMulti<true>().run(solver)
            : RunnerMulti<false>().run(solver);
    };

    return GlobalArgs.hardMode
        ? lambda.template operator()<false>()
        : lambda.template operator()<true>();

}

template <bool isEasyMode>
void Runner::precompute(const AnswersAndGuessesSolver<isEasyMode> &solver) {
    DEBUG("initialising...");

    START_TIMER(precompute);
    PatternGetterCached::buildCache();
    SolveForNIdeas::runOtherProgramIfRequired(GlobalArgs.otherProgram, false);
    NonLetterLookup::build();
    RemoveGuessesUsingNonLetterMask::buildLetterLookup();
    RemoveGuessesWithBetterGuessCache::init();
    EndGameDatabase(solver).init("list1");
    SolveForNIdeas::runOtherProgramIfRequired(GlobalArgs.otherProgram, true);

    END_TIMER(precompute);
}