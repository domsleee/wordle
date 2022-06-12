#pragma once
#include "Defs.h"
#include "GlobalState.h"
#include "LookupCacheEntry.h"

#include <map>
#include <string>
#include <filesystem>

struct EndGameAnalysis {
    static inline std::vector<GuessesVec> wordNum2EndGame = {};
    static inline std::vector<std::vector<std::pair<int, int>>> wordNum2EndGamePair = {};
    static inline int numEndGames = 0;
    static const int minEndGameCount = 4;
    static inline std::vector<std::vector<int>> endGames = {};
    static inline std::vector<std::vector<std::vector<BestWordResult>>> cache;

    static void init() {
        int i,j,nh=GlobalState.allAnswers.size();
        std::map<std::string, unsigned int> wcount;// Map from wildcarded string, e.g. ".arks", to number of occurrences
        std::map<std::string, int> w2e;    // Map from wildcarded string, e.g. ".arks", to endgame number
        
        for (std::string &s: GlobalState.allAnswers){
            for (j = 0; j < 5; j++){
                std::string t = getWildcard(s, j);
                if (wcount.count(t) == 0) wcount[t] = 0;
                wcount[t]++;
            }
        }
        
        wordNum2EndGame.resize(nh);
        wordNum2EndGamePair.resize(nh);
        endGames.assign(numEndGames, {});
        numEndGames = 0;
        for (i = 0; i < nh; i++){
            std::string s = GlobalState.allAnswers[i];
            for(j = 0; j < 5; j++){
                std::string t = getWildcard(s, j);
                if (wcount[t] >= minEndGameCount){
                    if (w2e.count(t) == 0) {
                        w2e[t] = numEndGames++;
                        endGames.resize(numEndGames);
                    }
                    wordNum2EndGame[i].push_back(w2e[t]);
                    wordNum2EndGamePair[i].push_back({w2e[t], endGames[w2e[t]].size()});
                    endGames[w2e[t]].push_back(i);
                }
            }
        }

        DEBUG("numEndGames: " << numEndGames);
    }

    static std::string getWildcard(const std::string &s, int j) {
        return s.substr(0,j) + "." + s.substr(j+1, 5-(j+1));
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
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());

        setDefaultCache();

        long long totalAmount = 0;
        for (auto &en: cache) for (auto &en2: en) totalAmount += en2.size();
        auto bar = SimpleProgress("endGameCache", totalAmount+1);
        bar.everyNth = 100;

        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (int e = 0; e < numEndGames; ++e) {
                bar.updateStatus(FROM_SS(" remDepth: " << (int)remDepth << ", e: " << getFrac(e, numEndGames) << ", sub: " << subsetHelp << ", sup: " << supersetHelp));
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
        }
        bar.dispose();
        END_TIMER(initEndGameAnalysisCache);
        writeCacheToFile(filename);
    }

    static void setDefaultCache() {
        cache.resize(GlobalArgs.maxTries-2); // remDepth = 0, 1, maxTries not populated
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            cache[remDepth-2].resize(numEndGames);
            for (int e = 0; e < numEndGames; ++e) {
                int maxV = getMaxV(e);
                BestWordResult def(-1, MAX_INDEX_TYPE);
                assertBounds(remDepth, e, maxV);
                cache[remDepth-2][e].assign(maxV+1, def);
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
        return cache[remDepth-2][e][mask];
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
        cache.at(remDepth-2).at(e).at(mask) = entry;
        return isNewEntry;
    }

    static void assertBounds(RemDepthType remDepth, int e, int mask) {
        assert(2 <= remDepth && remDepth <= GlobalArgs.maxTries-1);
        assert(0 <= e && e < numEndGames);
        assert(0 <= mask && mask <= getMaxV(e));
    }

    static void writeCacheToFile(const std::string &filename = "endGameData.dat") {
        START_TIMER(writeCacheToFile);
        std::ofstream fout(filename);
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (int e = 0; e < numEndGames; ++e) {
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

    static int getMaxV(int e) {
        return (1 << endGames[e].size()) - 1;
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
};
