#pragma once
#include "Defs.h"
#include "GlobalState.h"
#include "GlobalArgs.h"
#include "EndGameDefs.h"
#include "EndGameAnalysisHelpers.h"

#include <map>
#include <string>
#include <filesystem>

using namespace EndGameAnalysisHelpers;

struct EndGameAnalysis {
    static inline std::vector<std::vector<std::pair<int, int>>> wordNum2EndGamePair = {};
    static inline EndGameList endGames = {};
    static inline EndGameCache cache;

    static const int minEndGameCount = 4;

    static void initEndGames() {
        int nh=GlobalState.allAnswers.size();
        IndexType i;
        std::map<std::string, unsigned int> wcount;// Map from wildcarded string, e.g. ".arks", to number of occurrences
        std::map<std::string, int> w2e;    // Map from wildcarded string, e.g. ".arks", to endgame number
        
        for (std::string &s: GlobalState.allAnswers){
            for (std::string t: getWildcards(s)) {
                if (wcount.count(t) == 0) wcount[t] = 0;
                wcount[t]++;
            }
        }

        endGames = {};
        for (i = 0; i < nh; i++){
            std::string s = GlobalState.allAnswers[i];
            for (auto t: getWildcards(s)) {
                if (wcount[t] >= minEndGameCount){
                    if (w2e.count(t) == 0) {
                        w2e[t] = endGames.size();
                        endGames.resize(endGames.size()+1);
                    }
                    int e = w2e[t];
                    endGames[e].push_back(i);
                }
            }
        }
        
        populateWordNum2EndGamePair();

        DEBUG("numEndGames: " << endGames.size());
    }

    static void populateWordNum2EndGamePair() {
        wordNum2EndGamePair.assign(GlobalState.allGuesses.size(), {});
        for (std::size_t e = 0; e < endGames.size(); ++e) {
            populateWordNum2EndGamePair(e, endGames[e]);
        }
    }

    static void populateWordNum2EndGamePair(std::size_t e, const AnswersVec &answers) {
        for (std::size_t k = 0; k < answers.size(); ++k) {
            IndexType v = answers[k];
            wordNum2EndGamePair[v].push_back({e, k});
        }
    }

    static void printEndgame(int e) {
        DEBUG("endGame: " << e);
        std::vector<std::string> res = {};
        for (auto v: endGames[e]) res.push_back(GlobalState.reverseIndexLookup[v]);
        printIterable(res);
    }

    static std::vector<std::string> getWildcards(const std::string &s) {
        std::vector<std::string> res = {};
        for (int i = 0; i < 5; ++i) {
            std::string cop(s);
            cop[i] = '.';
            res.push_back(cop);
        }
        // for (int i = 0; i < 5; ++i) {
        //     for (int j = i+1; j < 5; ++j) {
        //         std::string cop(s);
        //         cop[i] = cop[j] = '.';
        //         res.push_back(cop);
        //     }
        // }
        // for (int i = 0; i < 5; ++i) {
        //     for (int j = i+1; j < 5; ++j) {
        //         for (int k = j+1; k < 5; ++k) {
        //             std::string cop(s);
        //             cop[i] = cop[j] = cop[k] = '.';
        //             res.push_back(cop);
        //         }
        //     }
        // }

        return res;
    }

    template <bool isEasyMode>
    static void initCache(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
        std::string filename = FROM_SS(
            "databases/endGame_"
            << GlobalState.allAnswers.size()
            << "_" << GlobalState.allGuesses.size()
            << "_" << GlobalArgs.maxWrong
            << "_" << GlobalArgs.maxTries
            << ".dat");
        if (std::filesystem::exists(filename)) {
            readFromCache(filename);
            return;
        }

        START_TIMER(initEndGameAnalysisCache);
        auto solver = nothingSolver;
        solver.disableEndGameAnalysis = true;
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());

        setDefaultCache();

        long long totalAmount = 0;
        for (auto &en: cache) for (auto &en2: en) totalAmount += en2.size();
        auto bar = SimpleProgress("endGameCache", totalAmount+1);
        bar.everyNth = 100;

        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                populateEntry(solver, bar, remDepth, e, guesses);
            }
        }
        bar.dispose();
        END_TIMER(initEndGameAnalysisCache);
        writeCacheToFile(filename);
    }

    template <bool isEasyMode>
    static void populateEntry(AnswersAndGuessesSolver<isEasyMode> &solver, SimpleProgress &bar, RemDepthType remDepth, std::size_t e, GuessesVec &guesses) {
        bar.updateStatus(FROM_SS(" remDepth: " << (int)remDepth << ", e: " << getFrac(e, (int)endGames.size()) << ", sub: " << subsetHelp << ", sup: " << supersetHelp));
        int maxV = getMaxV(e);
        for (int mask = maxV; mask >= 0; --mask) {
            AnswersVec answers = {};
            for (int k = 0; (1 << k) <= maxV; ++k) {
                if (mask & (1 << k)) {
                    answers.push_back(endGames[e][k]);
                }
            }
            if (getFromCache(remDepth, e, mask).numWrong != -1) continue;
            auto res = solver.minOverWordsLeastWrong(answers, guesses, remDepth, 0, solver.getDefaultBeta());
            if (setCache(bar, remDepth, e, mask, res)) {
                // if (res.numWrong >= solver.getDefaultBeta()) {
                //     // DEBUG("markAllParents??");
                //     markAllSupersets(bar, remDepth, e, mask, res);
                //     for (RemDepthType rDepth2 = remDepth - 1; rDepth2 >= 2; --rDepth2) {
                //         setCache(bar, rDepth2, e, mask, res);
                //         markAllSubsets(bar, rDepth2, e, mask, res);
                //     }
                // }
                if (res.numWrong == 0) {
                    markAllSubsets(bar, remDepth, e, mask, res);
                    for (RemDepthType rDepth2 = remDepth + 1; rDepth2 <= GlobalArgs.maxTries-1; ++rDepth2) {
                        setCache(bar, rDepth2, e, mask, res);
                        markAllSubsets(bar, rDepth2, e, mask, res);
                    }
                }
            }
        }
    }

    static void setDefaultCache() {
        cache.resize(GlobalArgs.maxTries-2); // remDepth = 0, 1, maxTries not populated
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            cache[getRemDepthIndex(remDepth)].resize(endGames.size());
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                int maxV = getMaxV(e);
                BestWordResult def(-1, MAX_INDEX_TYPE);
                assertBounds(remDepth, e, maxV);
                cache[getRemDepthIndex(remDepth)][e].assign(maxV+1, def);
            }
        }
    }

    inline static long long supersetHelp = 0, subsetHelp = 0;
    static void markAllSupersets(SimpleProgress &bar, RemDepthType remDepth, int e, int mask, const BestWordResult res) {
        int maxV = (1 << endGames[e].size()) - 1;
        for (int k = 0; (1 << k) <= maxV; ++k) {
            if (mask & (1 << k)) continue;
            auto newMask = mask | (1 << k);
            if (setCache(bar, remDepth, e, newMask, res)) {
                supersetHelp++;
                markAllSupersets(bar, remDepth, e, newMask, res);
            }
        }
    }

    static void markAllSubsets(SimpleProgress &bar, RemDepthType remDepth, int e, int mask, const BestWordResult res) {
        int maxV = getMaxV(e);
        for (int k = 0; (1 << k) <= maxV; ++k) {
            if ((mask & (1 << k)) == 0) continue;
            auto newMask = mask & (~(1 << k));
            if (setCache(bar, remDepth, e, newMask, res)) {
                subsetHelp++;
                markAllSubsets(bar, remDepth, e, newMask, res);
            }
        }
    }

    static BestWordResult getFromCache(RemDepthType remDepth, int e, int mask) {
        assertBounds(remDepth, e, mask);
        return cache[getRemDepthIndex(remDepth)][e][mask];
    }

    static bool setCache(SimpleProgress &bar, RemDepthType remDepth, int endGame, int mask, const BestWordResult entry) {
        auto isNewEntry = setCacheNoBar(remDepth, endGame, mask, entry);
        if (isNewEntry) bar.incrementAndUpdateStatus("");
        // assert(bar.maxA == countPopulatedEntries());
        return isNewEntry;
    }

    static long long countPopulatedEntries() {
        long long ct = 0;
        for (auto &en: cache) for (auto &en2: en) for (auto &res: en2) ct += res.numWrong != -1;
        return ct;
    }

    static bool setCacheNoBar(RemDepthType remDepth, int e, int mask, const BestWordResult entry) {
        assert(entry.numWrong != -1);
        auto existingEntry = getFromCache(remDepth, e, mask);
        auto isNewEntry = existingEntry.numWrong == -1;
        if (!isNewEntry) {
            assert(existingEntry.numWrong == entry.numWrong);
            return false;
        }
        assertBounds(remDepth, e, mask);
        cache.at(getRemDepthIndex(remDepth)).at(e).at(mask) = entry;
        return isNewEntry;
    }

    static void assertBounds(RemDepthType remDepth, int e, int mask) {
        assert(2 <= remDepth && remDepth <= GlobalArgs.maxTries-1);
        assert(0 <= e && e < (int)endGames.size());
        assert(0 <= mask && mask <= getMaxV(e));
    }

    static void writeCacheToFile(const std::string &filename = "endGameData.dat") {
        START_TIMER(writeCacheToFile);
        std::ofstream fout(filename);
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                int maxV = getMaxV(e);
                for (int mask = 0; mask <= maxV; ++mask) {
                    auto entry = getFromCache(remDepth, e, mask);
                    if (entry.numWrong <= 0) continue;
                    fout << (int)remDepth << " " << e << " " << mask << ": " << entry.numWrong << " " << (int)entry.wordIndex << '\n';
                }
            }
        }
        fout.close();
        END_TIMER(writeCacheToFile);
    }

    static void readFromCache(const std::string &filename = "endGameData.dat") {
        START_TIMER(endGameAnalysisReadFromCache);
        setDefaultCache();
        std::ifstream fin{filename};
        if (!fin.good()) {
            DEBUG("bad filename " << filename);
            exit(1);
        }

        int remDepth;
        int e, mask;
        char c;
        BestWordResult entry = {};
        long long ct = 0;
        while (fin >> remDepth >> e >> mask >> c >> entry.numWrong >> entry.wordIndex) {
            setCacheNoBar(remDepth, e, mask, entry);
            ++ct;
        }
        DEBUG("set " << ct << " entries");
        END_TIMER(endGameAnalysisReadFromCache);
    }

    static int getMaxV(int e) {
        return (1 << EndGameAnalysis::endGames[e].size()) - 1;
    }
};