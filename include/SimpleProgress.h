#pragma once
#include "../third_party/indicators/cursor_control.hpp"
#include "../third_party/indicators/progress_bar.hpp"

struct SimpleProgress {
    const std::string prefix;
    std::string lastSuffix = "";
    int64_t maxA = 0;
    const int64_t total;
    indicators::ProgressBar bar;
    int everyNth = 1;

    SimpleProgress(const std::string &prefix, int64_t total, bool isPrecompute=false)
    : prefix(prefix),
      total(total),
      bar(getBar(isPrecompute)) {
        updateStatus(", loading...");
      }

    void incrementAndUpdateStatus(const std::string &suffix = "", int amount = 1) {
        maxA += amount;
        if ((maxA % everyNth) != 0 && !isFinished()) return;
        if (suffix.size() == 0) {
            updateStatusInner(lastSuffix);
        } else {
            lastSuffix = suffix;
            updateStatusInner(suffix);
        }
    }

    void updateStatus(const std::string &suffix) {
        lastSuffix = suffix;
        updateStatusInner(suffix);
    }
    
    void dispose() {
        bar.set_progress(100);
    }

private:
    static indicators::ProgressBar getBar(bool isPrecompute) {
        return indicators::ProgressBar{
            indicators::option::BarWidth{10},
            indicators::option::Start{"["},
            indicators::option::Fill{"■"},
            indicators::option::Lead{"■"},
            indicators::option::Remainder{"-"},
            indicators::option::End{" ]"},
            indicators::option::ForegroundColor{isPrecompute ? indicators::Color::yellow : indicators::Color::cyan},
            indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
        };
    }

    void updateStatusInner(const std::string &suffix) {
        //DEBUG("update status..." << suffix);
        bar.set_option(indicators::option::PostfixText{prefix + " " + getFrac(maxA, total) + suffix});
        bar.set_progress(getIntegerPerc(maxA, total));
        if (!isFinished()) bar.print_progress();
    }

    bool isFinished() {
        return maxA == total;
    }
};