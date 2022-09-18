#pragma once
#include <array>
#include <string>
#include "Defs.h"
#include "../third_party/cxxopts.hpp"

template<class Enum>
Enum AssertOneOf(const cxxopts::ParseResult &result, const std::string &key, const EnumMap<Enum> &enumMap) {
    std::string val = result[key].as<std::string>();
    std::unordered_map<std::string, Enum> reverseMap = {};
    for (auto it: enumMap) reverseMap[it.second] = it.first;
    auto it = reverseMap.find(val);
    if (it == reverseMap.end()) {
        DEBUG("arg " << key << " '" << val << "' must be one of " << getEnumMapString(enumMap));
        exit(1);
    }
    return it->second;
}

template<class Enum>
std::string getEnumMapString(const EnumMap<Enum> &enumMap) {
    return FROM_SS("{" << getIterableString(getEnumMapStrings(enumMap), ",") << "}");
}

template<class Enum>
std::vector<std::string> getEnumMapStrings(const EnumMap<Enum> &enumMap) {
    std::vector<std::string> keys = {};
    for (auto it: enumMap) keys.push_back(it.second);
    return keys;
}