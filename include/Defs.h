#pragma once
#include "AttemptStateFaster.h"
#include "AttemptStateFastest.h"
#include "Util.h"
#include "UnorderedVector.h"

using AttemptStateToUse = AttemptStateFaster;

// m1pro: --max-tries 3 --max-incorrect 50 -r
// 52s: std::vector<IndexType>
// 28s: UnorderedVector<IndexType>
using TypeToUse = UnorderedVector<IndexType>;