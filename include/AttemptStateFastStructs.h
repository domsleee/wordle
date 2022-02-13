#pragma once
#include <array>
#include <vector>

using LetterCountNumberType = uint32_t;
using PositionLetterType = uint32_t;
using LetterMapType = uint32_t;


struct WordIndexData {
    const LetterCountNumberType letterCountNumber;
    const PositionLetterType positionalLetterNumber;
    const LetterMapType letterMap;
    const std::array<LetterMapType, 4> letterCountMap;
    WordIndexData(
        LetterCountNumberType letterCountNumber,
        PositionLetterType positionalLetterNumber,
        LetterMapType letterMap,
        const std::array<LetterMapType, 4> &letterCountMap
    )
    : letterCountNumber(letterCountNumber),
      positionalLetterNumber(positionalLetterNumber),
      letterMap(letterMap),
      letterCountMap(letterCountMap) {}
};

template<typename T>
struct ValueWithMask {
    T value = 0;
    T mask = 0;
    ValueWithMask() {}
    ValueWithMask(T value, T mask): value(value), mask(mask) {}
    //ValueWithMask<T>& operator=(const ValueWithMask<T> &classObj);
};

struct LetterToCheckLetterMap {
    uint8_t letterCountToCheck = 0;
    LetterMapType letterMap = 0;
    LetterToCheckLetterMap() {}
    LetterToCheckLetterMap(uint8_t letterCountToCheck, LetterMapType letterMap)
        : letterCountToCheck(letterCountToCheck),
          letterMap(letterMap) {}
};

using LetterMaxLimitType = uint32_t;
using WrongSpotPatternType = uint32_t;
struct GuessIndexPatternData {
    const LetterCountNumberType letterMinLimitNumber;
    const std::array<LetterMapType, 4> letterMinLimit;
    const ValueWithMask<PositionLetterType> rightSpotNumber;
    const std::array<LetterToCheckLetterMap, 2> letterMaxLimit;
    const uint8_t letterMaxLimitSize;
    const std::array<ValueWithMask<PositionLetterType>, 5> wrongSpotPattern;
    const uint8_t wrongSpotPatternSize;
    const int excludedLetterMap;
    //const int letterMaxLimitNumber;
    //const int64_t wrongSpotPatternNumber;

    GuessIndexPatternData(
        LetterCountNumberType letterMinLimitNumber,
        const std::array<LetterMapType, 4> &letterMinLimit,
        ValueWithMask<PositionLetterType> rightSpotNumber,
        const std::vector<LetterToCheckLetterMap> &letterMaxLimit,
        const std::vector<ValueWithMask<PositionLetterType>> &wrongSpotPattern,
        const int excludedLetterMap
    )
    : letterMinLimitNumber(letterMinLimitNumber),
      letterMinLimit(letterMinLimit),
      rightSpotNumber(rightSpotNumber),
      letterMaxLimit(buildArrayFromVec<LetterToCheckLetterMap, 2>(letterMaxLimit)),
      letterMaxLimitSize(letterMaxLimit.size()),
      wrongSpotPattern(buildArrayFromVec<ValueWithMask<PositionLetterType>, 5>(wrongSpotPattern)),
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
        // for (std::size_t i = 0; i < N; ++i) ret[i] = std::numeric_limits<T>::max();
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
