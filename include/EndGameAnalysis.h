#pragma once
#include "Defs.h"
#include "GlobalState.h"

#include <map>
#include <string>
#include <filesystem>

struct EndGameAnalysis {
    static inline std::vector<std::vector<std::pair<int, int>>> wordNum2EndGamePair = {};
    static inline std::vector<AnswersVec> endGames = {};
    static inline std::vector<bool> isOneWildcard = {};
    static inline std::vector<std::vector<std::vector<BestWordResult>>> cache;

    static const int minEndGameCount = 4;

    template <bool isEasyMode>
    static void init(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
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
                int ct = 0;
                for (char c: t) ct += (c == '.');
                if (wcount[t] >= minEndGameCount){
                    if (w2e.count(t) == 0) {
                        w2e[t] = endGames.size();
                        endGames.resize(endGames.size()+1);
                        isOneWildcard.resize(endGames.size());
                    }
                    int e = w2e[t];
                    // wordNum2EndGame[i].push_back(e);
                    // wordNum2EndGamePair[i].push_back({e, endGames[e].size()});
                    endGames[e].push_back(i);
                    isOneWildcard[e] = ct == 1;
                }
            }
        }
        std::vector<AnswersVec> filteredEndGames = getFilteredEndGames(nothingSolver);

        DEBUG("filtered endGames: " << getPerc(filteredEndGames.size(), endGames.size()));
        endGames = filteredEndGames;
        std::size_t maxEndGame = 0;
        
       // wordNum2EndGame.assign(nh, {});
        wordNum2EndGamePair.assign(nh, {});
        for (std::size_t e = 0; e < endGames.size(); ++e) {
            for (std::size_t k = 0; k < endGames[e].size(); ++k) {
                IndexType v = endGames[e][k];
                //wordNum2EndGame[v].push_back(e);
                wordNum2EndGamePair[v].push_back({e, k});
            }
            maxEndGame = std::max(maxEndGame, endGames[e].size());
        }

        DEBUG("numEndGames: " << endGames.size() << ", maxEndGame: " << maxEndGame);
    }

    static void printEndgame(int e) {
        DEBUG("endGame: " << e);
        std::vector<std::string> res = {};
        for (auto v: endGames[e]) res.push_back(GlobalState.reverseIndexLookup[v]);
        printIterable(res);
    }

    template <bool isEasyMode>
    static std::vector<AnswersVec> getFilteredEndGames(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
        std::set<int> filteredEndGames = {};
        std::set<AnswersVec> resEndGames = {};
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        int remDepthGte3 = 0;
        long long totalOneWildcard = 0, totalToEvaluate = 0;

        for (std::size_t e = 0; e < endGames.size(); ++e) {
            if (isOneWildcard[e]) {
                totalOneWildcard++;
                filteredEndGames.insert(e);
                resEndGames.insert(endGames[e]);
                if (endGames[e].size() >= 20) {
                    DEBUG("LSDKFJSDLKJF: " << e << " " << endGames[e].size() << ", " << getBoolVal(isOneWildcard[e]));
                    printIterable(endGames[e]);
                    exit(1);
                }
            }
        }

        const int groupSize = 19;
        int myMax=0;
        for (RemDepthType remDepth = 4; remDepth >= 2; remDepth--) {
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                if (!isOneWildcard[e]) {
                    totalToEvaluate += std::max(0, (int)endGames[e].size()-groupSize);
                    myMax = std::max(myMax, (int)endGames[e].size());
                }
            }
        }
        DEBUG("myMax: " << myMax);

        auto bar = SimpleProgress("EndGameFilter", totalToEvaluate);

        for (RemDepthType remDepth = 4; remDepth >= 2; remDepth--) {
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                if (filteredEndGames.contains(e)) continue;

                auto solver = nothingSolver;
                solver.disableEndGameAnalysis = true;
                
                const auto &endGame = endGames[e];
                if (endGame.size() < groupSize) continue;
                for (std::size_t i = 0; i < endGame.size()-groupSize; ++i) {
                    AnswersVec answers = {};
                    for (std::size_t j = i; j < i+groupSize; ++j) answers.push_back(endGame[j]);
                    auto res = solver.minOverWordsLeastWrong(answers, guesses, remDepth, 0, solver.getDefaultBeta());
                    if (res.numWrong > 0) {
                        filteredEndGames.insert(e);
                        resEndGames.insert(answers);
                        //DEBUG("INSERT: " << resEndGames.size() << ": " << answers.size());
                        if (answers.size() > 20) { DEBUG("SDLFJDSKL:F"); exit(1); }
                        remDepthGte3 += remDepth >= 3;
                        i += groupSize;
                        bar.incrementAndUpdateStatus("", groupSize-1);
                    }
                    bar.incrementAndUpdateStatus("");
                }
                if (remDepth == 2 && filteredEndGames.size() >= 2000) break;
                
            }
        }

        DEBUG("percent remDepthGte3 " << getPerc(remDepthGte3, filteredEndGames.size()) << ", totalOneWildcard: " << totalOneWildcard);

        return {resEndGames.begin(), resEndGames.end()};
    }

    static std::vector<std::string> getWildcards(const std::string &s) {
        std::vector<std::string> res = {};
        for (int i = 0; i < 5; ++i) {
            std::string cop(s);
            cop[i] = '.';
            res.push_back(cop);
        }
        for (int i = 0; i < 5; ++i) {
            for (int j = i+1; j < 5; ++j) {
                std::string cop(s);
                cop[i] = cop[j] = '.';
                res.push_back(cop);
            }
        }
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
        }
        bar.dispose();
        END_TIMER(initEndGameAnalysisCache);
        writeCacheToFile(filename);
    }

    static void setDefaultCache() {
        cache.resize(GlobalArgs.maxTries-2); // remDepth = 0, 1, maxTries not populated
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            cache[remDepth-2].resize(endGames.size());
            for (std::size_t e = 0; e < endGames.size(); ++e) {
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

