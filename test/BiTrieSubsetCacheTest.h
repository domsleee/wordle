#pragma once
#include "../third_party/catch.hpp"
#include "../include/SubsetCache/BiTrieSubsetCache.h"
#include "TestDefs.h"

BiTrieSubsetCache prepareCache(const std::vector<AnswersVec> &cacheEntries) {
    auto cache = BiTrieSubsetCache(0);
    GuessesVec g = {};

    for (const auto &entry: cacheEntries) {
        cache.setSubsetCache(entry, g, 0, 1);
    }
    return cache;
}

TEST_CASE("BiTrieSubsetCache") {
    auto cache = prepareCache({
        {1, 2, 3}
    });
    AnswersVec Q = {1, 2, 3, 4};
    auto lb = cache.getLbFromAnswerSubsets(Q, GuessesVec());
    REQUIRE_MESSAGE(lb == 1, "Q is a harder problem than cache1, so it should have lb");
}

TEST_CASE("BiTrieSubsetCache_harder") {
    auto cache = prepareCache({
        {1082, 3784, 3787, 4359, 4362, 5315, 5547, 5548, 6934, 6940, 6944, 7473, 11394, 11397, 12522, 12524, 12526, 12780}
    });
    AnswersVec Q = {1082, 3784, 3787, 4359, 4362, 5040, 5042, 5044, 5315, 5547, 5548, 6934, 6940, 6944, 7473, 11394, 11397, 12146, 12522, 12524, 12526, 12780};
    auto lb = cache.getLbFromAnswerSubsets(Q, GuessesVec());
    REQUIRE_MESSAGE(lb == 1, "Q is a harder problem than cache1, so it should have lb");
}


TEST_CASE("BiTrieSubsetCache_bidirectional") {
    auto cache = prepareCache({
        {1082, 3784, 3787, 4359, 4362, 5315, 5547, 5548, 6934, 6940, 6944, 7473, 11394, 11397, 12522, 12524, 12526, 12780}
    });
    AnswersVec Q = {1082, 3784, 3787, 4359, 4362, 5040, 5042, 5044, 5315, 5547, 5548, 6934, 6940, 6944, 7473, 11394, 11397, 12146, 12522, 12524, 12526, 12780};
    auto lb = cache.getLbFromAnswerSubsetsBidirectional(Q, GuessesVec());
    REQUIRE_MESSAGE(lb == 1, "Q is a harder problem than cache1, so it should have lb");
}

TEST_CASE("BiTrieSubsetCache_bidirecitonal1") {
    auto cache = prepareCache({
        {741, 781, 854, 897, 2658, 3270, 3277, 3568, 3588, 3646, 3652, 3658, 4158, 4169, 4231, 4272, 4801, 4822, 4828, 4915, 4929, 5452, 5463, 5655, 12017, 12058, 12069, 12696, 12884}
    });
    AnswersVec Q = {741, 781, 854, 897, 2658, 3270, 3277, 3568, 3588, 3646, 3652, 3658, 4801, 4822, 4828, 4915, 4929, 5452, 5463, 5655, 12017, 12058, 12069, 12884};
    auto lb = cache.getLbFromAnswerSubsetsBidirectional(Q, GuessesVec());
    REQUIRE_MESSAGE(lb == 0, "cache1 is not a subset of Q, so it should not have lb");
}