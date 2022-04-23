#pragma once
#include "Util.h"
#include "UnorderedVector.h"
#include <execution>

struct AttemptStateFaster;
using AttemptStateToUse = AttemptStateFaster;

// m1pro: --max-tries 3 --max-incorrect 50 -r
// 52s: std::vector<IndexType>
// 28s: UnorderedVector<IndexType>
using UnorderedVec = UnorderedVector<IndexType>;
using TypeToUse = UnorderedVec;


constexpr std::size_t NUM_PATTERNS = intPow(3, WORD_LENGTH);
