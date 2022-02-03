#pragma once
#include <string>
#include <cassert>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>

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

inline std::vector<std::string> getFirstNWords(const std::vector<std::string> &words, int n) {
    std::vector<std::string> res = {};
    for (auto i = 0; i < n; ++i) res.push_back(words[i]);
    return res;
}


inline std::vector<std::string> getWordsOfLength(const std::vector<std::string> &words, std::size_t len) {
    std::vector<std::string> wordsOfLength = {};
    for (auto word: words) {
        if (word.size() == len) wordsOfLength.push_back(word);
    }
    return wordsOfLength;
}
