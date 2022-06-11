#pragma once
#include "Defs.h"
struct LookupCacheEntry {
    IndexType guessForLb, guessForUb;
    int lb, ub;
    LookupCacheEntry(IndexType guessForLb, IndexType guessForUb, int lb, int ub)
        : guessForLb(guessForLb),
          guessForUb(guessForUb),
          lb(lb),
          ub(ub) {}
    LookupCacheEntry() {}
};