#pragma once
#include "../third_party/indicators/cursor_control.hpp"
#include "../third_party/indicators/progress_bar.hpp"

struct SimpleProgress {
    const std::string prefix;
    std::mutex mutex;
    std::string lastSuffix = "";
    int64_t maxA = 0;
    const int64_t total;
    indicators::ProgressBar bar;

    SimpleProgress(const std::string &prefix, int64_t total, bool isPrecompute=false)
    : prefix(prefix),
      total(total),
      bar(getBar(isPrecompute)) {
          updateStatus(", loading...");
      }

    void incrementAndUpdateStatus(const std::string &suffix = "") {
        const std::lock_guard<std::mutex> lock(mutex);
        ++maxA;
        if (suffix.size() == 0) {
            updateStatusInner(lastSuffix);
        } else {
            updateStatusInner(suffix);
        }
        
    }

    void updateStatus(const std::string &suffix) {
        const std::lock_guard<std::mutex> lock(mutex);
        updateStatusInner(suffix);
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
        bar.set_progress(getIntegerPerc(maxA, total));
        bar.set_option(indicators::option::PostfixText{prefix + " " + getFrac(maxA, total) + suffix});
    }
};