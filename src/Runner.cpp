#include "../include/Runner.h"

#include "../include/Util.h"
#include "../include/PatternGetterCached.h"
#include "../include/RunnerMulti.h"
#include "../include/Verifier.h"
#include "../include/EndGameAnalysis.h"


int Runner::run() {
    auto answers = readFromFile(GlobalArgs.answers);
    auto guesses = readFromFile(GlobalArgs.guesses);

    guesses = mergeAndUniq(answers, guesses);
    if (GlobalArgs.reduceGuesses) {
        answers = getFirstNWords(answers, GlobalArgs.numToRestrict);
        guesses = answers;
    }

    GlobalState = _GlobalState(guesses, answers);

    auto lambda = [&]<bool isEasyMode, bool isGetLowestAverage>() -> bool {
        auto solver = AnswersAndGuessesSolver<isEasyMode, isGetLowestAverage>(GlobalArgs.maxTries);

        START_TIMER(precompute);
        RemoveGuessesWithNoLetterInAnswers::buildClearGuessesInfo();
        PatternGetterCached::buildCache();
        EndGameAnalysis::init();

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

    auto lambda2 = [&]<bool isEasyMode>() -> bool {
        return GlobalArgs.isGetLowestAverage
            ? lambda.template operator()<isEasyMode, true>()
            : lambda.template operator()<isEasyMode, false>();
    };

    return GlobalArgs.hardMode
        ? lambda2.template operator()<false>()
        : lambda2.template operator()<true>();

}