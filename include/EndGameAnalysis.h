#pragma once
#include "Defs.h"
#include "GlobalState.h"

#include <map>
#include <string>

struct EndGameAnalysis {
    static inline std::vector<GuessesVec> wordNum2EndGame = {};
    static inline int numEndGames = 0;
    static const int minEndGameCount = 200;

    static void init() {
        int i,j,nh=GlobalState.allAnswers.size();
        std::map<std::string, uint> wcount;// Map from wildcarded string, e.g. ".arks", to number of occurrences
        std::map<std::string, int> w2e;    // Map from wildcarded string, e.g. ".arks", to endgame number
        
        for (std::string &s: GlobalState.allAnswers){
            for (j = 0; j < 5; j++){
                std::string t = getWildcard(s, j);
                if (wcount.count(t) == 0) wcount[t] = 0;
                wcount[t]++;
            }
        }
        
        wordNum2EndGame.resize(nh);
        numEndGames = 0;
        for (i = 0; i < nh; i++){
            std::string s = GlobalState.allAnswers[i];
            for(j = 0; j < 5; j++){
                std::string t = getWildcard(s, j);
                if (wcount[t] >= minEndGameCount){
                    if (w2e.count(t) == 0) w2e[t] = numEndGames++;
                    wordNum2EndGame[i].push_back(w2e[t]);
                }
            }
        }

        DEBUG("numEndGames: " << numEndGames);
    }

    static std::string getWildcard(const std::string &s, int j) {
        return s.substr(0,j) + "." + s.substr(j+1, 5-(j+1));
    }
};
