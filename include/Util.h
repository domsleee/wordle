#pragma once
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <array>

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


static const int MAX_TRIES = 6;
static const int NUM_WORDS = 2400;
static const int MAX_NUM_GUESSES = 2400;
static const char NULL_LETTER = '-';
using WordProbPair = std::pair<double, std::string>;
using MinLetterType = std::array<int8_t, 26>;


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

