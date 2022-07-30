#pragma once
#include "Util.h"
#include "Defs.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include "EndGameAnalysis/EndGameAnalysisHelpers.h"
#include "EndGameAnalysis/EndGameDefs.h"

const std::string folderName = "endGameCache";

using namespace EndGameAnalysisHelpers;

template <bool isEasyMode>
struct EndGameDatabase {
    const AnswersAndGuessesSolver<isEasyMode> &nothingSolver;

    EndGameDatabase(const AnswersAndGuessesSolver<isEasyMode> &nothingSolver)
        : nothingSolver(nothingSolver) {}
    
    void init(
        const std::string &listName // list1
    ) {
        EndGameAnalysis::endGames = readEndGamesList(listName);
        populateEndGameCacheEntries();
        EndGameAnalysis::populateWordNum2EndGamePair();

        DEBUG("#endGames: " << EndGameAnalysis::endGames.size());
    }

    std::vector<AnswersVec> readEndGamesList(const std::string &listName) {
        std::string filename = FROM_SS(getEndGameListsFolder() << "/" << listName);
        if (!std::filesystem::exists(filename)) {
            DEBUG("filename not found, get default endgamelist");
            EndGameAnalysis::initEndGames();
            writeEndGamesLists(listName, EndGameAnalysis::endGames);
            return EndGameAnalysis::endGames;
        }

        std::ifstream fin(filename);

        int numEndGames;
        fin >> numEndGames;
        std::vector<AnswersVec> endGames = {};
        endGames.resize(numEndGames);

        for (auto &endGame: endGames) {
            int endGameSize;
            fin >> endGameSize;
            endGame.resize(endGameSize);
            for (int j = 0; j < endGameSize; ++j) {
                fin >> endGame[j];
            }
        }
        
        return endGames;
    }

    void writeEndGamesLists(const std::string &listName, const EndGameList &endGameList) {
        std::string filename = FROM_SS(getEndGameListsFolder() << "/" << listName);
        auto fout = safeFout(filename);
        fout << endGameList.size() << "\n";
        for (auto &endGame: endGameList) {
            writeEndGamesList(fout, endGame);
            fout << '\n';
        }
    }

    static void writeEndGamesList(std::ostream &fout, const AnswersVec &endGame) {
        fout << endGame.size();
        for (auto w: endGame) fout << ' ' << w;
    }

    void populateEndGameCacheEntries() {
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        EndGameAnalysis::setDefaultCache();
        long long totalAmount = 0;
        for (auto &en: EndGameAnalysis::cache) for (auto &en2: en) totalAmount += en2.size();
        auto bar = SimpleProgress("endGameCache2", totalAmount+1);
        bar.everyNth = 100;

        auto solver = nothingSolver;
        solver.disableEndGameAnalysis = true;

        for (RemDepthType remDepth = 2; remDepth <= GlobalArgs.maxTries-1; ++remDepth) {
            for (std::size_t e = 0; e < EndGameAnalysis::endGames.size(); ++e) {
                auto entry = readEntryFromFile(remDepth, e);
                if (entry.size() > 0) {
                    EndGameAnalysis::cache[getRemDepthIndex(remDepth)][e] = entry;
                    auto maxV = EndGameAnalysis::getMaxV(e);
                    bar.incrementAndUpdateStatus("", maxV+1);
                } else {
                    EndGameAnalysis::populateEntry(solver, bar, remDepth, e, guesses);
                    writeEntryToFile(remDepth, e, EndGameAnalysis::cache[getRemDepthIndex(remDepth)][e]);
                }
            }
        }

        bar.dispose();
    }

    std::vector<BestWordResult> readEntryFromFile(RemDepthType remDepth, std::size_t e) {
        auto powersetsFilename = getEndGamePowersets(remDepth, e);

        if (!std::filesystem::exists(powersetsFilename)) {
            return {};
        }

        std::ifstream fin(powersetsFilename);
        int numMasks = EndGameAnalysis::getMaxV(e);
        std::vector<BestWordResult> result(numMasks+1);
        for (int i = 0; i <= numMasks; ++i) {
            fin >> result[i].numWrong >> result[i].wordIndex;
        }
        return result;
    }

    std::string getEndGamePowersets(RemDepthType remDepth, std::size_t e) {
        std::string powersetsFilename = std::to_string(remDepth);
        for (IndexType w: EndGameAnalysis::endGames[e]) powersetsFilename += FROM_SS("_" << w);
        std::string filename = FROM_SS(getEndGamesFolder() << "/" << powersetsFilename);
        return filename;
    }

    void writeEntryToFile(RemDepthType remDepth, std::size_t e, const std::vector<BestWordResult> &res) {
        auto powersetsFilename = getEndGamePowersets(remDepth, e);

        if (std::filesystem::exists(powersetsFilename)) {
            DEBUG("warning, overriding: " << powersetsFilename);
        }

        auto fout = safeFout(powersetsFilename);
        int numMasks = EndGameAnalysis::getMaxV(e);
        if (static_cast<int>(res.size()) != (numMasks+1)) {
            DEBUG("very confused! " << res.size() << " VS " << (numMasks+1));
        }
        for (int i = 0; i <= numMasks; ++i) {
            auto entry = EndGameAnalysis::cache[getRemDepthIndex(remDepth)][e][i];
            fout << ' ' << entry.numWrong << ' ' << entry.wordIndex;
        }
    }

    std::string getEndGameListsFolder() {
        return FROM_SS(folderName << "/endGameLists_" << GlobalState.allAnswers.size() << "_" << GlobalState.allGuesses.size());
    }

    std::string getEndGamesFolder() {
        return FROM_SS(folderName << "/endGamePowersets_" << GlobalState.allAnswers.size() << "_" << GlobalState.allGuesses.size());
    }
};