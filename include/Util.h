#pragma once
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <array>
#include <cmath>
#define DEBUG(x) std::cout << x << '\n';
#define assertm(expr, msg) assert(((void)(msg), (expr)))

#define START_TIMER(name) auto timer_##name = std::chrono::steady_clock::now()
#define END_TIMER(name)                                                      \
    {                                                                        \
        std::cout << "Time elapsed: "                                        \
                  << (std::chrono::duration_cast<std::chrono::microseconds>( \
                          std::chrono::steady_clock::now() - timer_##name)   \
                          .count()) /                                        \
                         1000000.0                                           \
                  << "\n\n";                                                 \
    }


using IndexType = uint16_t;

static const int MAX_TRIES = 3;
static const int NUM_WORDS = 2400;
static const int MAX_NUM_GUESSES = 13000;
static const char NULL_LETTER = '-';
static const int MAX_LETTER_LIMIT_MAX = 10;
const int WORD_LENGTH = 5;
const int NUM_PATTERNS = pow(3, WORD_LENGTH);
using MinLetterType = std::array<int8_t, 26>;

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

inline std::vector<IndexType> getVector(std::size_t size, std::size_t offset) {
    std::vector<IndexType> res(size);
    for (std::size_t i = 0; i < size; ++i) {
        res[i] = i + offset;
    }
    return res;
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

inline std::vector<std::string> mergeAndSort(const std::vector<std::string> &a, const std::vector<std::string> &b) {
    std::set<std::string> res;
    for (auto w: a) res.insert(w);
    for (auto w: b) res.insert(w);
    return {res.begin(), res.end()};
}

#include <sstream>
#include <iomanip>

inline std::string getPerc(long long a, long long b) {
    std::stringstream ss;
    ss << a << "/" << b << " (" << std::setprecision(2) << std::fixed << 100.00*a/b << "%)";
    return ss.str();
}

