#pragma once
#include "Util.h"
#include "UnorderedVector.h"
#include <execution>
#include <vector>
#include <array>
#include <map>

struct AttemptStateFaster;
using AttemptStateToUse = AttemptStateFaster;

// m1pro: --max-tries 3 --max-incorrect 50 -r
// 52s: std::vector<IndexType>
// 28s: UnorderedVector<IndexType>
using UnorderedVec = UnorderedVector<IndexType>;
using TypeToUse = UnorderedVec;

template <int ID>
struct SafeVec : public std::vector<IndexType> {
    using std::vector<IndexType>::vector;
};

enum string_id {ANSWERS, GUESSES};

using AnswersVec = SafeVec<ANSWERS>;
using GuessesVec = SafeVec<GUESSES>;

constexpr std::size_t NUM_PATTERNS = intPow(3, WORD_LENGTH);

template <bool isEasyMode>
struct AnswersAndGuessesSolver;

static constexpr int INF_INT = (int)1e8;
static constexpr double INF_DOUBLE = 1e8;
static constexpr int INDENT = 2;

enum OtherProgram {
    blank,
    any3in2,
    any4in2,
    any6in3
};
template <class E>
using EnumMap = std::map<E, std::string>;

static const EnumMap<OtherProgram> otherProgramMap = { {blank, ""}, {any3in2, "any3in2"}, {any4in2, "any4in2"}, {any6in3, "any6in3"} };