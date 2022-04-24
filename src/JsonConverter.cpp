#include "../include/JsonConverter.h"
#include "../third_party/json.hpp"
#include "../include/PatternIntHelpers.h"
SolutionModel parseModel(const nlohmann::json &j);
nlohmann::json toJson(const SolutionModel &solutionModel);

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
    if (j.contains("next")) {
        for (const auto &jj: j["next"].items()) {
            auto key = jj.key();
            const auto newModel = parseModel(jj.value());
            res.next[key] = std::make_shared<SolutionModel>(std::move(newModel));
        }
    }
    return res;
}

void JsonConverter::toFile(const SolutionModel &solutionModel, const std::string &file) {
    auto j = toJson(solutionModel);
    std::ofstream o(file);
    o << std::setw(2) << j << std::endl;
}

nlohmann::json toJson(const SolutionModel &solutionModel) {
    nlohmann::json j;
    j["guess"] = solutionModel.guess;
    j["next"] = nlohmann::json({});
    for (const auto &item: solutionModel.next) {
        if (item.first == "++++++") j["next"][item.first] = item.second->guess;
        else j["next"][item.first] = toJson(*item.second);
    }
    return j;
}

