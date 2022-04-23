#include "../include/JsonConverter.h"
#include "../third_party/json.hpp"
#include "../include/JsonConverterPatternHelpers.h"

SolutionModel parseModel(const nlohmann::json &j);

SolutionModel JsonConverter::fromFile(const std::string &file) {
    std::ifstream i(file);
    nlohmann::json j;
    i >> j;
    return parseModel(j);
}

SolutionModel parseModel(const nlohmann::json &j) {
    SolutionModel res = {};
    res.guess = j["guess"];
    if (j.contains("guessesLeft")) {
        res.guess = j["guessesLeft"].get<int>();
    }
    if (j.contains("next")) {
        for (auto &jj: j["next"].items()) {
            auto key = JsonConverterPatternHelpers::emojiToPatternInt(jj.key().get<std::wstring>());
            res.next[key] = parseModel(jj.value());
        }
    }
    return res;
}

void JsonConverter::toFile(const std::string &file) {
    // ??
}
