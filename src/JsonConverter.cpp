#include "../include/JsonConverter.h"
#include "../third_party/json.hpp"
#include "../include/PatternIntHelpers.h"
SolutionModel parseModel(const nlohmann::json &j);

SolutionModel JsonConverter::fromFile(const std::string &file) {
    std::ifstream i(file);
    nlohmann::json j;
    i >> j;
    return parseModel(j);
}

SolutionModel parseModel(const nlohmann::json &j) {
    SolutionModel res = {};
    if (j.is_string()) {
        res.guess = j;
        return res;
    }
    if (j.contains("guess")) {
        res.guess = j["guess"];
    }
    if (j.contains("guessesLeft")) {
        DEBUG("HAS GUESSES LEFT??");
        res.guessesLeft = j["guessesLeft"].get<int>();
        res.possibilitiesLeft = j["possibilities"].size();
    }
    if (j.contains("next")) {
        for (const auto &jj: j["next"].items()) {
            auto key = jj.key();
            const auto newModel = parseModel(jj.value());
            res.next[key] = std::make_shared<SolutionModel>(std::move(newModel));
        }
    }
    return res;
}

void JsonConverter::toFile(const std::string &file) {
    // ??
}
