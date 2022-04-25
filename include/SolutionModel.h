// https://github.com/andrew-t/gaming-wordle/blob/main/easy.json
#pragma once
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>
#include "Util.h"

struct SolutionModel;

struct SolutionModel {
    std::string guess = "";
    std::unordered_map<std::string, std::shared_ptr<SolutionModel>> next = {};

    void mergeFromOtherModel(std::shared_ptr<SolutionModel> other, std::string path = "$") {
        if (other->guess != guess) {
            DEBUG("how to merge this?? non-matching guess " << guess << " vs " << other->guess << " path: " << path);
            exit(1);
        }
        for (const auto &item: next) {
            if (other->next.contains(item.first)) {
                item.second->mergeFromOtherModel(other->next.at(item.first), path + "." + item.first);
            }
        }
        for (const auto &item: other->next) {
            if (!next.contains(item.first)) {
                next[item.first] = item.second;
            }
        }
    }

    static std::set<std::string> getAllGuessesFromNext(const SolutionModel& model) {
        std::set<std::string> result = {};
        getAllGuessesFromNextInner(result, model);
        return result;
    }

    static void getAllGuessesFromNextInner(std::set<std::string> &result, const SolutionModel &model) {
        if (model.next.size() == 0) {
            result.insert(model.guess);
        }
        for (const auto &item: model.next) {
            getAllGuessesFromNextInner(result, *item.second);
        }
    }

    std::shared_ptr<SolutionModel> getOrCreateNext(const std::string &patternStr) {
        if (next.contains(patternStr)) {
            return next[patternStr];
        }
        return next[patternStr] = std::make_shared<SolutionModel>();
    }

    friend std::ostream& operator<<(std::ostream& os, const SolutionModel& model) {
        os << "{\nguess: " << model.guess << "\n";
        os << "next: {\n";
        for (auto k: model.next) os << k.first << ": " << *k.second << "\n";
        os << "}";
        return os;
    }

};
