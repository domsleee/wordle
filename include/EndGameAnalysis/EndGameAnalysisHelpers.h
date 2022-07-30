#pragma once
#include "Defs.h"
#include "Util.h"

namespace EndGameAnalysisHelpers {
    static RemDepthType getRemDepthIndex(RemDepthType remDepth) {
        return remDepth-2;
    }
}