#pragma once
#include "RunnerMulti.h"
#include <vector>

struct Runner {
    static std::vector<RunnerMultiResult> run();
    template <bool isEasyMode>
    static void precompute(const AnswersAndGuessesSolver<isEasyMode> &solver);
};

