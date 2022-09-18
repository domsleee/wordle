#pragma once
#include "Util.h"
#include "Defs.h"
#include "GlobalArgs.h"
#include "GlobalState.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include <bitset>
#include "SolveForNIdeas/SolveForNHelpers.h"
#include "SolveForNIdeas/Any4In2.h"
#include "SolveForNIdeas/Any3In2.h"
#include "SolveForNIdeas/Any6In3.h"


struct SolveForNIdeas {
    static bool requiresCache(OtherProgram otherProgram) {
        return otherProgram == OtherProgram::any6in3;
    }

    static void runOtherProgramIfRequired(OtherProgram otherProgram, bool cacheLoaded) {
        if (requiresCache(otherProgram) && !cacheLoaded) return;
        switch (otherProgram) {
            case OtherProgram::any3in2: checkCanAny3BeSolvedIn2(); break;
            case OtherProgram::any4in2: checkCanAny4BeSolvedIn2(); break;
            case OtherProgram::any6in3: checkCanAny6BeSolvedIn3(); break;
            default: return;
        }
        exit(1);
    }

    static void checkCanAny3BeSolvedIn2() {
        Any3In2::checkCanAny3BeSolvedIn2();
    }

    // no good batty,patty,tatty,fatty
    // no good paper,racer,waver,wafer
    static void checkCanAny4BeSolvedIn2() {
        auto solver = AnswersAndGuessesSolver<true>(2);
        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        int64_t total = nChoosek(answers.size(), 4);
        Any4In2::precompute();
        std::set<std::set<IndexType>> nogood = {};

        auto bar = SimpleProgress(FROM_SS("canAny4BeSolvedIn2: " << total), GlobalState.allAnswers.size());
        int64_t ct = 0, skipped = 0, numGroups = 0;
        std::ofstream fout(getAny4In2File());
        // answers = {0};
        std::vector<int> transformResults(answers.size(), 0);
        std::mutex lock;

        if (GlobalArgs.forceSequential) {
            Any4In2::any4In2Fastest(solver, bar, 0, fout, lock, ct, skipped, numGroups);
            return;
        }

        std::transform(
            std::execution::par_unseq,
            answers.begin(),
            answers.end(),
            transformResults.begin(),
            [&](const IndexType a1) -> int {
                Any4In2::any4In2Fastest(solver, bar, a1, fout, lock, ct, skipped, numGroups);
                //any4Faster(solver, bar, a1, fout, lock, ct, skipped, nogood);
                return 0;
            }
        );

        DEBUG("magnificient (4)"); exit(1);
    }

    static std::string getAny4In2File() {
        return FROM_SS("results/any4in2-" << GlobalState.allAnswers.size() << "-" << GlobalState.allGuesses.size() << ".txt");
    }

    static void checkCanAny6BeSolvedIn3() {
        auto any4In2File = getAny4In2File();
        if (!std::filesystem::exists(any4In2File)) {
            DEBUG("NO SUCH FILE: " << any4In2File); exit(1);
        }
        if (GlobalArgs.minCache < 6) {
            DEBUG("WARNING: recommend setting --min-cache, you will smash your cache");
        }
        std::ifstream fin(any4In2File);
        std::string line;
        std::vector<AnswersVec> list4NotIn2 = {};
        while (std::getline(fin,line)) {
            std::vector<std::string> spl = split(line, ',');
            AnswersVec my4NotIn2 = {};
            for (auto myStr: spl) my4NotIn2.push_back(GlobalState.getIndexForWord(myStr));
            if (my4NotIn2.size() != 4) ERROR("EXPECTED SIZE: " << 4 << " VS " << my4NotIn2.size() << ", " << line);
            list4NotIn2.emplace_back(my4NotIn2);
        }
        Any6In3::any6In3(list4NotIn2); exit(1);
    }
};