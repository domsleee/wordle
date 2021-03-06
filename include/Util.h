#pragma once
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <array>
#include <cmath>
#include <bitset>

#define DEBUG(x) std::cout << x << '\n';
#define assertm(expr, msg) assert(((void)(msg), (expr)))

#define START_TIMER(name) auto timer_##name = std::chrono::steady_clock::now()
#define END_TIMER(name)                                                      \
    {                                                                        \
        std::cout << "Timer " << #name << ": "                               \
                  << (std::chrono::duration_cast<std::chrono::microseconds>( \
                          std::chrono::steady_clock::now() - timer_##name)   \
                          .count()) /                                        \
                         1000000.0                                           \
                  << "\n";                                                 \
    }


using IndexType = uint16_t;
using PatternType = uint8_t;
using RemDepthType = uint8_t;
using FastModeType = uint8_t;

static const int MAX_NUM_GUESSES = 12992;
static const int NUM_WORDS = 12992;
static const char NULL_LETTER = '-';
static const int MAX_LETTER_LIMIT_MAX = 10;

const int WORD_LENGTH = 5;
using MinLetterType = std::array<int8_t, 26>;

static constexpr RemDepthType TRIES_FAILED = std::numeric_limits<RemDepthType>::max();
static constexpr std::size_t MAX_SIZE_VAL = std::numeric_limits<std::size_t>::max();
static constexpr IndexType MAX_INDEX_TYPE = std::numeric_limits<IndexType>::max();
static constexpr uint32_t MAX_UINT32_T = std::numeric_limits<uint32_t>::max();

template <class T>
int getIndex(const std::vector<T> &vec, const T& val) {
    auto it = std::find(vec.begin(), vec.end(), val);
    if (it == vec.end()) return -1;
    return std::distance(vec.begin(), it);
}

template <typename T>
static T safeMultiply(T a, T b) {
    T x = a * b;
    if (a != 0 && x / a != b) {
        DEBUG("multiplication overflow " << (long long)a << " x " << (long long)b << ", typeid: " << typeid(a).name());
        //throw std::runtime_error("fail");
        exit(1);
    }
    return x;
}

template <typename Vec = std::vector<IndexType>>
inline Vec getVector(std::size_t size, std::size_t offset = 0) {
    Vec res(size);
    for (std::size_t i = 0; i < size; ++i) {
        res[i] = i + offset;
    }
    return res;
}
 
#include<sstream>

#define FROM_SS(x) static_cast<std::ostringstream &&>((std::ostringstream() << x)).str()


inline std::string getBoolVal(bool v) {
    return v ? "Yes" : "No";
}

inline std::string toLower(const std::string &s) {
    std::string ret = "";
    for (auto c: s) {
        ret += std::tolower(c);
    }
    return ret;
}

inline std::vector<std::string> readFromFile(const std::string &filename) {
    std::ifstream fin{filename};
    if (!fin.good()) {
        DEBUG("bad filename " << filename);
        exit(1);
    }
    std::vector<std::string> res;
    std::string s;
    while (fin >> s) res.push_back(s);
    return res;
}

inline std::vector<std::string> getFirstNWords(const std::vector<std::string> &words, std::size_t n) {
    std::vector<std::string> res = {};
    n = std::min(n, words.size());
    for (std::size_t i = 0; i < n; ++i) res.push_back(words[i]);
    return res;
}


inline std::vector<std::string> getWordsOfLength(const std::vector<std::string> &words, std::size_t len) {
    std::vector<std::string> wordsOfLength = {};
    for (auto word: words) {
        if (word.size() == len) wordsOfLength.push_back(word);
    }
    return wordsOfLength;
}

#include <set>

inline std::vector<std::string> mergeAndUniq(const std::vector<std::string> &a, const std::vector<std::string> &b) {
    std::vector<std::string> res = {};
    std::set<std::string> seen = {};
    for (auto w: a) {
        res.push_back(w);
        seen.insert(w);
    }
    for (auto w: b) {
        if (seen.count(w)) continue;
        res.push_back(w);
    }
    return res;
}

#include <sstream>
#include <iomanip>

inline double safeDivide(int64_t a, int64_t b) {
    if (b == 0) return std::numeric_limits<double>::infinity();
    return (double)a/b;
}

inline std::string getFrac(int64_t a, int64_t b) {
    std::stringstream ss;
    ss << a << "/" << b;
    return ss.str();
}

inline std::string getPerc(int64_t a, int64_t b) {
    std::stringstream ss;
    ss << a << "/" << b << " (" << std::setprecision(2) << std::fixed << 100.00*safeDivide(a, b) << "%)";
    return ss.str();
}

inline double getIntegerPerc(int64_t a, int64_t b) {
    return 100.00 * safeDivide(a, b);
}

inline std::string getDivided(int64_t a, int64_t b) {
    std::stringstream ss;
    ss << a << "/" << b << "=" << std::setprecision(2) << std::fixed << safeDivide(a, b);
    return ss.str();
}

// https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int
inline constexpr int64_t intPow(int64_t base, int64_t exp) {
    if (exp == 0) return 1;  // base case;
    int64_t temp = intPow(base, exp/2);
    return (exp % 2 == 0)
        ? temp * temp
        : (base * temp * temp);
}

template <typename T>
inline void checkWordSetSize(const std::string &desc, std::size_t arraySize) {
    auto wordSetSize = T().size();
    auto optimalSize = ((arraySize + 63)/64)*64;
    if (arraySize > wordSetSize) {
        DEBUG("ERROR: " << desc << " too big. Optimal: " << optimalSize << ". currently: " << arraySize << " vs " << wordSetSize);
        exit(1);
    }

    if (wordSetSize != optimalSize) {
        DEBUG("WARNING: optimal for " << desc << " is " << optimalSize << ", currently: " << wordSetSize);
    }

}

template <typename T>
inline void printIterable(const T &iterable) {
    for (auto el: iterable) DEBUG(el);
}

