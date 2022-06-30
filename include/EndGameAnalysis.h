#pragma once
#include "Defs.h"
#include "GlobalState.h"

#include <map>
#include <string>
#include <filesystem>

struct EndGameAnalysis {
    static inline std::vector<std::vector<std::pair<int, int>>> wordNum2EndGamePair = {};
    static inline std::vector<AnswersVec> endGames = {};
    static inline std::vector<std::vector<std::vector<BestWordResult>>> cache;

    // ??
    static inline std::vector<bool> isOneWildcard = {};

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
        
        populateWordNum2EndGamePair();

        DEBUG("numEndGames: " << endGames.size() << ", maxEndGame: " << maxEndGame);
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

    template <bool isEasyMode>
    static std::vector<AnswersVec> getFilteredEndGames(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
        std::set<int> filteredEndGames = {};
        std::set<AnswersVec> resEndGames = {};
        long long totalOneWildcard = 0;

        for (std::size_t e = 0; e < endGames.size(); ++e) {
            if (!isOneWildcard[e]) continue;
            totalOneWildcard++;
            filteredEndGames.insert(e);
            resEndGames.insert(endGames[e]);
        }

        auto r2 = getFilteredEndGamesDifferentToOneWildcard(nothingSolver);
        for (auto &v: r2) resEndGames.insert(v);
        return {resEndGames.begin(), resEndGames.end()};
    }

    template <bool isEasyMode>
    static std::set<AnswersVec> getFilteredEndGamesDifferentToOneWildcard(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
        std::set<AnswersVec> resEndGames = {};
        auto solver = nothingSolver;
        solver.disableEndGameAnalysis = true;
        
        std::vector<std::vector<AnswersVec>> groupsThatAreDifferentToOneWildcard = {};
        for (std::size_t e = 0; e < endGames.size(); ++e) {
            if (isOneWildcard[e]) continue;
            groupsThatAreDifferentToOneWildcard.push_back(getGroupsDiffToOne(e));
        }

        long long totalToEvaluate = 0;
        for (const auto &grp: groupsThatAreDifferentToOneWildcard) {
            totalToEvaluate += 1 * grp.size();
        }

        auto bar = SimpleProgress("EndGameFilter_DiffOne", totalToEvaluate);
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        for (const auto &grp: groupsThatAreDifferentToOneWildcard) {
            for (const auto &answerVec: grp) {
                auto res = solver.minOverWordsLeastWrong(answerVec, guesses, 3, 0, solver.getDefaultBeta());
                if (res.numWrong > 0) {
                    // good anakin, gooood
                    resEndGames.insert(answerVec);
                    break;
                }
                bar.incrementAndUpdateStatus(FROM_SS(" endGames: " << resEndGames.size()    ));
            }
        }
        bar.dispose();
        DEBUG("EndGameFilter_DiffOne: " << resEndGames.size());
        return resEndGames;
    }

    template <bool isEasyMode>
    static std::set<AnswersVec> getFilteredEndGamesSlidingWindow(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver) {
        std::set<int> filteredEndGames = {};
        std::set<AnswersVec> resEndGames = {};
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        int remDepthGte3 = 0;
        long long totalToEvaluate = 0;
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
                if (filteredEndGames.contains(e) || isOneWildcard[e]) continue;

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

        return resEndGames;
    }

    static std::vector<AnswersVec> getGroupsDiffToOne(int e) {
        std::vector<AnswersVec> res = {};
        AnswersVec vec = {};
        if (endGames[e].size() > 40) return res;
        std::vector<int> endGameCt(endGames.size());
        populateWordNum2EndGamePair();
        std::size_t maxI = 50000;
        getGroupsDiffToOne(res, endGameCt, vec, 0, e, maxI);
        DEBUG("getGroupsDiffToOne: " << e << ", size: " << res.size());
        return res;
    }

    static void getGroupsDiffToOne(std::vector<AnswersVec> &res, std::vector<int> &endGameCt, AnswersVec &vec, const std::size_t i, const int e, std::size_t &maxI) {
        //DEBUG("call res:" << res.size() << ", vec:" << vec.size() << ", i: " << i << ", e: " << e << ", endGames[e]: " << endGames[e].size() << ", maxI: " << maxI);
        if (res.size() > 50000) return;
        static const std::size_t groupSize = 19;
        static const int maxSimilarToOne = 7;

        assert(vec.size() <= groupSize);
        if (vec.size() > groupSize) { DEBUG("FAIL"); exit(1); }

        if (i == endGames[e].size()) {
            if (vec.size() == groupSize) {   
                res.push_back(vec);
                std::size_t newE = endGameCt.size();
                endGameCt.resize(newE + 1);
                endGameCt[newE] = vec.size();
                populateWordNum2EndGamePair(newE, vec);
                // maxI = i-groupSize+1;
            }
            return;
        }

        if (i > maxI) return;
        if (vec.size() + (endGames[e].size() - i) < groupSize) return;

        getGroupsDiffToOne(res, endGameCt, vec, i+1, e, maxI); // don't take it
        // take it
        if (vec.size() == groupSize) return;
        IndexType ind = endGames[e][i];
        int M = 0;
        for (auto [e2, _]: wordNum2EndGamePair[ind]) {
            if (!isOneWildcardOrAdded(e2)) continue;
            M = std::max(M, ++endGameCt[e2]);
        }

        if (M <= maxSimilarToOne) {
            // do recursion
            vec.push_back(ind);
            getGroupsDiffToOne(res, endGameCt, vec, i+1, e, maxI);
            vec.pop_back();
        }

        for (auto [e2, _]: wordNum2EndGamePair[ind]) {
            if (!isOneWildcardOrAdded(e2)) continue;
            --endGameCt[e2];
        }
    }

    static bool isOneWildcardOrAdded(std::size_t e) {
        return isOneWildcard[e] || (e >= endGames.size());
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

