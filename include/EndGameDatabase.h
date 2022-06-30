#pragma once
#include "Util.h"
#include "Defs.h"
#include "EndGameAnalysis.h"
const std::string folderName = "endGameCache";


template <bool isEasyMode>
struct EndGameDatabase {
    const AnswersAndGuessesSolver<isEasyMode> &nothingSolver;
    const std::string wordListId;

    EndGameDatabase(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver)
        : nothingSolver(nothingSolver),
          wordListId(getWordListId()) {}
    
    void populateEndGamesFromList(
        const std::string &listName // list1
    ) {
        std::string filename = FROM_SS(getEndGameListsFolder() << "/" << listName);
        std::ifstream fin(filename);

        int numEndGames;
        fin >> numEndGames;
        EndGameAnalysis::endGames.resize(0);
        EndGameAnalysis::endGames.resize(numEndGames);

        for (auto &endGame: EndGameAnalysis::endGames) {
            int endGameSize;
            fin >> endGameSize;
            endGame.resize(endGameSize);
            for (int j = 0; j < endGameSize; ++j) {
                fin >> endGame[j];
            }
        }

        populateEndGameCacheEntries();

        EndGameAnalysis::populateWordNum2EndGamePair();
    }

    std::string getEndGameListsFolder() {
        return FROM_SS(folderName << "/endGameLists_" << GlobalState.allAnswers.size() << "_" << GlobalState.allGuesses.size());
    }

    std::string getEndGamesFolder() {
        return FROM_SS(folderName << "endGames_" << GlobalState.allAnswers.size() << "_" << GlobalState.allGuesses.size());
    }


    void populateEndGameCacheEntries() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        EndGameAnalysis::setDefaultCache();
        long long totalAmount = 0;
        for (auto &en: EndGameAnalysis::cache) for (auto &en2: en) totalAmount += en2.size();
        auto bar = SimpleProgress("endGameCache2", totalAmount+1);
        bar.everyNth = 100;

        auto solver = nothingSolver;
        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (std::size_t e = 0; e < endGames.size(); ++e) {
                auto entry = readEntryFromFile(remDepth, e);
                if (entry.size() > 0) {
                    cache[remDepth][e] = entry;
                } else {
                    EndGameAnalysis::populateEntry(solver, bar, remDepth, e, guesses);
                    writeEntryToFile(remDepth, e, cache[remDepth][e]);
                }
            }
        }

    }

    std::vector<BestWordResult> readEntryFromFile(RemDepthType remDepth, std::size_t e) {
        std::string wordListFilename = std::to_string(remDepth);
        for (IndexType w: EndGameAnalysis::endGames[e]) wordListFile += FROM_SS("_" << w);
        std::string filename = FROM_SS(getEndGamesFolder() << "/" << wordListFilename);

        if (!std::filesystem::exists(filename)) {
            return {};
        }

        std::ifstream fin(filename);
        const auto &endGame = EndGameAnalysis::endGames[e];
        int numMasks = (1 << endGame.size()) - 1;
        std::vector<BestWordResult> result(numMasks+1);
        for (int i = 0; i <= numMasks; ++i) {
            int sz;
            fin >> sz;
            result[i].resize(sz);
            for (int j = 0; j < sz; ++j) fin >> result[i][j].numWrong >> result[i][j].wordIndex;
        }
    }

    void writeEntryToFile(RemDepthType remDepth, std::size_t e, const std::vector<BestWordResult> &res) {
        std::string wordListFilename = std::to_string(remDepth);
        for (IndexType w: EndGameAnalysis::endGames[e]) wordListFile += FROM_SS("_" << w);
        std::string filename = FROM_SS(getEndGamesFolder() << "/" << wordListFilename);

        if (std::filesystem::exists(filename)) {
            DEBUG("warning, overriding: " << filename);
        }

        std::ofstream fout(filename);
        const auto &endGame = EndGameAnalysis::endGames[e];
        int numMasks = (1 << endGame.size()) - 1;
        if (res.size() != numMasks) {
            DEBUG("very confused! " << res.size() << " VS " << numMasks);
        }
        for (int i = 0; i <= numMasks; ++i) {
            int sz = res[i].size();
            fout << sz;
            for (int j = 0; j < sz; ++j) fuot << res[i][j].numWrong << res[i][j].wordIndex;
        }
    }
};