#pragma once
#include <array>
#include <vector>

using LetterCountNumberType = uint32_t;
using PositionLetterType = uint64_t;


struct WordIndexData {
    const LetterCountNumberType letterCountNumber;
    const PositionLetterType positionalLetterNumber;
    const int letterMap;
    WordIndexData(LetterCountNumberType letterCountNumber, PositionLetterType positionalLetterNumber, int letterMap)
        : letterCountNumber(letterCountNumber),
          positionalLetterNumber(positionalLetterNumber),
          letterMap(letterMap) {}
};

using LetterMaxLimitType = uint32_t;
using WrongSpotPatternType = uint32_t;
struct GuessIndexPatternData {
    const LetterCountNumberType letterMinLimitNumber;
    const PositionLetterType rightSpotNumber;
    const std::array<LetterMaxLimitType, 5> letterMaxLimit;
    const uint8_t letterMaxLimitSize;
    const std::array<WrongSpotPatternType, 5> wrongSpotPattern;
    const uint8_t wrongSpotPatternSize;
    const int excludedLetterMap;
    //const int letterMaxLimitNumber;
    //const int64_t wrongSpotPatternNumber;

    GuessIndexPatternData(
        LetterCountNumberType letterMinLimitNumber,
        PositionLetterType rightSpotNumber,
        const std::vector<LetterMaxLimitType> &letterMaxLimit,
        const std::vector<WrongSpotPatternType> &wrongSpotPattern,
        const int excludedLetterMap
    )
    : letterMinLimitNumber(letterMinLimitNumber),
      rightSpotNumber(rightSpotNumber),
      letterMaxLimit(buildArrayFromVec<LetterMaxLimitType, 5>(letterMaxLimit)),
      letterMaxLimitSize(letterMaxLimit.size()),
      wrongSpotPattern(buildArrayFromVec<WrongSpotPatternType, 5>(wrongSpotPattern)),
      wrongSpotPatternSize(wrongSpotPattern.size()),
      excludedLetterMap(excludedLetterMap)
      //letterMaxLimitNumber(combine<int>(letterMaxLimit)),
      //wrongSpotPatternNumber(combine<int64_t>(wrongSpotPattern))
    {}

    template <class T, std::size_t N>
    std::array<T, N> buildArrayFromVec(const std::vector<T> &vec) {
        std::array<T, N> ret = {};
        if (vec.size() > N) {
            DEBUG("incorrect size for array, given " << vec.size() << ", expected: " << N);
            exit(1);
        }
        for (std::size_t i = 0; i < vec.size(); ++i) {
            ret[i] = vec[i];
        }
        return ret;
    }

    template <typename T>
    T combine(const std::vector<int> &vec) {
        T res = 1;
        for (auto v: vec) res = safeMultiply(res, v);
        return res;
    }
};

struct WordAndPatternData {
    MinLetterType letterMinLimit = {}, letterMaxLimit = {};
    std::string wrongSpotPattern = std::string(WORD_LENGTH, NULL_LETTER);
    std::string rightSpotPattern = std::string(WORD_LENGTH, NULL_LETTER);
};
