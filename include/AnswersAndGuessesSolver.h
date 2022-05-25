#pragma once
#include "AttemptState.h"
#include "AttemptStateFaster.h"
#include "AnswersAndGuessesResult.h"
#include "GlobalState.h"

#include "PatternGetter.h"
#include "WordSetUtil.h"
#include "BestWordResult.h"
#include "AnswersAndGuessesKey.h"
#include "Defs.h"
#include "UnorderedVector.h"
#include "RemoveGuessesWithBetterGuessCache.h"

#include "EndGameAnalysis.h"
#include "PerfStats.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) 

static constexpr int INF_INT = (int)1e8;
static constexpr double INF_DOUBLE = 1e8;

template <bool isEasyMode, bool isGetLowestAverage=false>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(RemDepthType maxTries)
        : maxTries(maxTries)
        {}

    const RemDepthType maxTries;
    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, BestWordResult> getGuessCache = {};
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, int> lbGuessCache = {};
    long long cacheMiss = 0, cacheHit = 0, lbCacheMiss = 0, lbCacheHit = 0;
    std::vector<std::array<std::array<int, NUM_PATTERNS>, MAX_NUM_GUESSES>> lbCache3d;
    std::vector<std::array<AnswersVec, NUM_PATTERNS>> myEquivCache;
    PerfStats stats;

    static const int isEasyModeVar = isEasyMode;

    AnswersAndGuessesResult solveWord(const std::string &answer, std::shared_ptr<SolutionModel> solutionModel) {
        if (std::find(GlobalState.allAnswers.begin(), GlobalState.allAnswers.end(), answer) == GlobalState.allAnswers.end()) {
            DEBUG("word not a possible answer: " << answer);
            exit(1);
        }

        IndexType firstWordIndex;

        auto answers = getVector<AnswersVec>(GlobalState.allAnswers.size());
        auto guesses = getVector<GuessesVec>(GlobalState.allGuesses.size());
        if (startingWord == "") {
            DEBUG("guessing first word with " << (int)maxTries << " tries...");
            auto firstPr = getGuessFunctionDecider(answers, guesses, maxTries, getDefaultBeta());
            DEBUG("first word: " << GlobalState.reverseIndexLookup[firstPr.wordIndex] << ", known numWrong: " << firstPr.numWrong);
            firstWordIndex = firstPr.wordIndex;
        } else {
            firstWordIndex = GlobalState.getIndexForWord(startingWord);
        }
        auto answerIndex = GlobalState.getIndexForWord(answer);
        return solveWord(answerIndex, solutionModel, firstWordIndex, answers, guesses);
    }

    AnswersAndGuessesResult solveWord(IndexType answerIndex, std::shared_ptr<SolutionModel> solutionModel, IndexType firstWordIndex, AnswersVec &answers, GuessesVec &guesses) {
        if (lbCache3d.size() == 0 && isGetLowestAverage) lbCache3d.resize(10);
        if (myEquivCache.size() == 0) {
            myEquivCache.assign(10, {});
            for (int remDepth = 0; remDepth < 10; ++remDepth) {
                for (std::size_t i = 0; i < NUM_PATTERNS; ++i) {
                    myEquivCache[remDepth][i].reserve(NUM_WORDS);
                }
            }
        }
        AnswersAndGuessesResult res = {};
        res.solutionModel = solutionModel;

        auto getter = PatternGetterCached(answerIndex);
        auto state = AttemptStateToUse(getter);
        auto currentModel = res.solutionModel;
        
        IndexType guessIndex = firstWordIndex;
        for (res.tries = 1; res.tries <= maxTries; ++res.tries) {
            currentModel->guess = GlobalState.reverseIndexLookup[guessIndex];
            if (currentModel->guess == "") {
                DEBUG("FAILED: " << guessIndex); exit(1);
            }
            if (guessIndex == answerIndex) {
                currentModel->next["+++++"] = std::make_shared<SolutionModel>();
                currentModel->next["+++++"]->guess = currentModel->guess;
                return res;
            }
            if (res.tries == maxTries) break;

            makeGuess(answers, guesses, state, guessIndex);
            /*auto it = std::find(answers.begin(), answers.end(), answerIndex);
            if (it == answers.end()) {
                DEBUG("the actual answer " << answerIndex << " was removed from answers after guess " << guessIndex << "!"); exit(1);
            }*/
            //DEBUG("result:")

            auto pr = getGuessFunctionDecider(answers, guesses, maxTries-res.tries, getDefaultBeta());
            if (res.tries == 1) res.firstGuessResult = pr;
            const auto patternInt = getter.getPatternIntCached(guessIndex);

            GUESSESSOLVER_DEBUG("NEXT GUESS: " << GlobalState.reverseIndexLookup[pr.wordIndex] <<  ",patternInt: " << patternInt << ", numWrong: " << pr.numWrong << " numAnswers: " << answers.size());

            const auto patternStr = PatternIntHelpers::patternIntToString(patternInt);
            currentModel = currentModel->getOrCreateNext(patternStr);

            guessIndex = pr.wordIndex;
        }
        res.tries = TRIES_FAILED;
        return res;
    }


    void makeGuess(AnswersVec &answers, GuessesVec &guesses, const AttemptStateToUse &state, IndexType guessIndex) const {
        const auto myGuessPatternInt = state.patternGetter.getPatternIntCached(guessIndex);
        std::erase_if(answers, [&](const auto answerIndex) {
            return PatternGetterCached::getPatternIntCached(answerIndex, guessIndex) != myGuessPatternInt;
        });
        /*
        std::size_t ind = 0;
        for (auto answerIndex: answers) {
            const auto patternInt = state.patternGetter.getPatternIntCached(answerIndex);
            if (patternInt == myGuessPatternInt) {
                answers[ind++] = answerIndex;
            }
        }
        answers.resize(ind);*/
        if constexpr (!isEasyMode) {
            for (std::size_t i = 0; i < answers.size(); ++i) guesses[i] = answers[i];
            guesses.resize(answers.size());
        }
    }

    BestWordResult calcSortVectorAndGetMinNumWrongFor2(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        std::array<IndexType, NUM_PATTERNS> &equiv,
        std::array<int64_t, MAX_NUM_GUESSES> &sortVec,
        const RemDepthType remDepth,
        int beta)
    {
        //DEBUG("called depth: " << GlobalArgs.maxTries - remDepth);
        BestWordResult minNumWrongFor2 = {INF_INT, 0};
        //auto &cacheEntry = minNumWrongFor2Cache.try_emplace(cacheKey.state.wsAnswers, std::unordered_map<IndexType, P>()).first->second;
        auto &count = equiv;
        int nh = answers.size();

        //if (remDepth == 2) DEBUG(guesses.size() << "," << answers.size());

        for (const auto guessIndex: guesses) {
            // const auto [numWrongFor2, sortVal] = calcSortVectorAndGetMinNumWrongFor2(guessIndex, answers, count, remDepth);
            int numWrongFor2 = 0, innerLb = 0, s2 = 0;
            int maxC = 0;
            count.fill(0);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                auto &c = count[patternInt];
                ++c;
                innerLb += 2 - (c == 1); // Assumes remdepth>=3
                s2 += 2*c - 1;
                numWrongFor2 += c > 1;
                if (remDepth == 2 && numWrongFor2 >= beta) break;
                if (c > maxC) maxC = c;
            }

            if (remDepth == 2 && numWrongFor2 < minNumWrongFor2.numWrong) {
                minNumWrongFor2 = {numWrongFor2, guessIndex};
                //assert(numWrongFor2 < beta);
                if (numWrongFor2 < beta) {
                    beta = numWrongFor2;
                }
            }
            
            if (numWrongFor2 == 0) {
                return {0, guessIndex};
            }

            if (remDepth > 2) {
                if (remDepth > maxC) return {0, guessIndex};
                innerLb -= count[NUM_PATTERNS - 1];
                auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
                sortVec[guessIndex] = sortVal;
            }
        }

        return minNumWrongFor2;
    }
    

    static std::pair<int, int64_t> calcSortVectorAndGetMinNumWrongFor2(
        const IndexType guessIndex,
        const AnswersVec &answers,
        std::array<IndexType, NUM_PATTERNS> &count,
        const RemDepthType remDepth=6)
    {
        int numWrongFor2 = 0, innerLb = 0, s2 = 0;
        int nh = answers.size(), maxC = 0;
        count.fill(0);
        for (const auto answerIndex: answers) {
            const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
            int c = (++count[patternInt]);
            auto lbVal = 2 - (c == 1); // Assumes remdepth>=3
            innerLb += lbVal;
            s2 += 2*c - 1;
            numWrongFor2 += c > 1;
            if (c > maxC) maxC = c;
        }
        
        innerLb -= count[NUM_PATTERNS - 1];
        auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
        //if (remDepth > maxC) return {0, sortVal};
        return {numWrongFor2, sortVal};
    }

// private:
    inline BestWordResult getGuessFunctionDecider(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        const int beta) {
        if constexpr (isGetLowestAverage) {
            return getGuessForLowestAverage(answers, guesses, remDepth, beta);
        } else {
            //return getGuessForLeastWrong(answers, guesses, remDepth, beta);
            return minOverWordsLeastWrong(answers, guesses, remDepth, 0, beta);
        }
    }

    BestWordResult minOverWordsLeastWrong(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, FastModeType fastMode, int beta) {
        assertm(remDepth != 0, "no tries remaining");
        stats.entryStats[GlobalArgs.maxTries-remDepth][0]++;

        if (answers.size() == 0) return {0, 0};
        if (answers.size() == 1) return {0, answers[0]};
        if (remDepth >= answers.size()) {
            auto m = *std::min_element(answers.begin(), answers.end());
            return {0, m};
        }
        if (remDepth == 1) {
            // no info from last guess
            auto m = *std::min_element(answers.begin(), answers.end());
            return {static_cast<int>(answers.size())-1, m};
        }

        stats.entryStats[GlobalArgs.maxTries-remDepth][1]++;
        if (fastMode == 1) return {-1, 0};

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }
        stats.entryStats[GlobalArgs.maxTries-remDepth][2]++;

        std::array<IndexType, NUM_PATTERNS> equiv;
        auto &count = equiv;
        count.fill(0);
        stats.tick(4445);
        for (const auto answerIndexForGuess: answers) {
            int maxC = 0;
            for (const auto actualAnswer: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualAnswer, answerIndexForGuess);
                int c = (++count[patternInt]);
                maxC = std::max(maxC, c);
            }
            if (maxC < remDepth) {
                stats.tock(4445);
                return setCacheVal(key, {0, answerIndexForGuess});
            }
        }
        stats.tock(4445);

        if (fastMode == 2) return {-1, 0};

        std::array<int64_t, MAX_NUM_GUESSES> sortVec;
        const int depth = GlobalArgs.maxTries - remDepth;

        GuessesVec guessesCopy;
        stats.tick(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));
        const GuessesVec& guessesToUse = remDepth == 2
            ? RemoveGuessesWithBetterGuessCache::getCachedGuessesVec(answers)
            : (guessesCopy = RemoveGuessesWithBetterGuessCache::buildGuessesVecWithoutRemovingAnyAnswer(guesses, answers));
        stats.tock(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));

        stats.tick(depth*5000 + answers.size());
        stats.tick(200 + depth);
        //if (stats.ncpu[200+depth] >= 2000) return {0, answers[0]};
        BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(answers, guessesToUse, equiv, sortVec, remDepth, beta);
        stats.tock(200 + GlobalArgs.maxTries-remDepth);
        stats.tock(depth*5000 + answers.size());
        if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
            bool exact = remDepth > 2 || minNumWrongFor2.numWrong < beta;
            if (exact) setCacheVal(key, minNumWrongFor2);
            else setLbCacheVal(key, minNumWrongFor2.numWrong);
            
            return minNumWrongFor2;
        }

        stats.entryStats[GlobalArgs.maxTries-remDepth][3]++;

        const bool shouldSort = true;
        if (shouldSort) std::sort(guessesCopy.begin(), guessesCopy.end(), [&](IndexType a, IndexType b) { return sortVec[a] < sortVec[b]; });

        // full search for remDepth >= 3
        BestWordResult res = getDefaultBestWordResult();
        int endGameLb = endGameAnalysis(answers, guessesCopy, key, remDepth, beta);
        if (endGameLb >= beta) {
            return {beta, answers[0]};
        }
        res.wordIndex = answers[0];
        std::size_t numGuessIndexesToCheck = std::min(guessesCopy.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        bool exact = false;
        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto possibleGuess = guessesCopy[myInd];
            auto r = sumOverPartitionsLeastWrong(answers, guessesCopy, remDepth-1, possibleGuess, beta);
            if (r < beta) {
                exact = true;
            }
            if (r < res.numWrong) {
                res = {r, possibleGuess};
                beta = std::min(r, beta);
            }
            if (r == 0) break;
        }

        if (exact) setCacheVal(key, res);
        else setLbCacheVal(key, res.numWrong);
        return res;
    }

    int sumOverPartitionsLeastWrong(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        const IndexType guessIndex,
        int beta
    )
    {
        assertm(remDepth != 0, "no tries remaining");
        if (answers.size() == 0) return 0;

        int lb[NUM_PATTERNS] = {0}, n = 0;
        PatternType indexToPattern[NUM_PATTERNS];
        std::array<AnswersVec, NUM_PATTERNS> myEquiv{};

        for (const auto answerIndex: answers) {
            const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
            myEquiv[patternInt].push_back(answerIndex);
        }
        int totalWrongForGuess = 0;
        for (PatternType s = 0; s < NUM_PATTERNS; ++s) {
            std::size_t sz = myEquiv[s].size();
            if (sz == 0) continue;
            if (s == NUM_PATTERNS) {
                lb[s] = 0;
                continue;
            }
            auto innerRes = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 1, beta - totalWrongForGuess);
            if (innerRes.numWrong != -1) {
                lb[s] = innerRes.numWrong;
            } else {
                lb[s] = 0;
                indexToPattern[n++] = s;
            }

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) return totalWrongForGuess;
        }

        int m = 0;
        for (int i = 0; i < n; ++i) {
            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            auto innerRes = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 2, beta - totalWrongForGuess);
            if (innerRes.numWrong != -1) {
                lb[s] = innerRes.numWrong;
            } else {
                auto lbKey = getCacheKey(myEquiv[s], guesses, remDepth-1);
                lb[s] = getLbCache(lbKey);
                indexToPattern[m++] = s;
            }

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) return totalWrongForGuess;
        }

        n = m;

        int score[243];
        for (int i = 0; i < n; ++i) {
            int s = indexToPattern[i];
            score[s] = myEquiv[s].size();
        }
        std::sort(indexToPattern, indexToPattern+n, [&score](auto s0, auto s1) -> bool { return score[s0] < score[s1];});

        for (int i = 0; i < n; ++i) {
            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            const auto pr = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 0, beta - totalWrongForGuess);
            totalWrongForGuess = std::min(totalWrongForGuess + pr.numWrong, INF_INT);
            if (totalWrongForGuess >= beta) {
                someLbHack(guesses, guessIndex, remDepth, lb, s, myEquiv);
                someLbHack2(guesses, guessIndex, remDepth, totalWrongForGuess, s, myEquiv);
                return totalWrongForGuess;
            }
        }

        return totalWrongForGuess;
    }

    int endGameAnalysis(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const AnswersAndGuessesKey<isEasyMode> &cacheKey,
        RemDepthType remDepth,
        int beta
    )
    {
        std::vector<int> endCount(EndGameAnalysis::numEndGames, 0);
        int e, biggestendgame = -1, mx = 0;

        for (auto answerIndex: answers) for (auto e: EndGameAnalysis::wordNum2EndGame[answerIndex]) endCount[e]++;
        for (e = 0; e < EndGameAnalysis::numEndGames; e++) {
            if (endCount[e] > mx) {
                mx = endCount[e];
                biggestendgame = e;
            }
        }
        if (biggestendgame >= 0 && remDepth >= 1 && mx-1 > remDepth-1) {
            // DEBUG("checking end game...");
            AnswersVec liveEndGame = {};
            for(auto answerIndex: answers) {
                for (auto e: EndGameAnalysis::wordNum2EndGame[answerIndex]) {
                    if (e == biggestendgame) liveEndGame.push_back(answerIndex);
                }
            }
            int mult[6]={0};
            std::pair<int, IndexType> indexThatAchievesLb = {0, 0};
            for(auto guessIndex: guesses){
                std::vector<int> seen(243, 0);
                int n = 0;
                for(auto answerIndex: liveEndGame) {
                    auto s = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                    n += (seen[s] == 0);
                    seen[s] = 1;
                }
                assert(n >= 1 && n <= 6); mult[n-1]++;
                if (n > indexThatAchievesLb.first) {
                    indexThatAchievesLb = {n, guessIndex};
                }
            }
            int n, r = remDepth - 1;
            int sum = 0;
            for (n = 5; n > 0 && r > 0; n--) {
                int r1 = std::min(r, mult[n]);
                sum += r1*n;
                r -= r1;
            }
            auto minNumWrong = mx - 1 - sum;
            if (minNumWrong > 0) setLbCacheVal(cacheKey, minNumWrong, indexThatAchievesLb.second);
            if (minNumWrong >= beta) {
                //static int endGameHit = 0;
                //DEBUG("end game hit..." << ++endGameHit);
                return minNumWrong;
            }

            /*if (liveEndGame.size() < answers.size()) {
                auto v = minOverWordsLeastWrong(liveEndGame, guesses, remDepth, 0, beta);
                if(v.numWrong >= beta) {
                    DEBUG("end sub lookup hit...");
                    setLbCacheVal(cacheKey, v.numWrong, v.wordIndex);
                    return v.numWrong;
                }
            }*/
        }
        return 0;
    }

    void someLbHack(const GuessesVec &guesses, const IndexType guessIndex, RemDepthType remDepth, const int *lb, PatternType s, const std::array<AnswersVec, NUM_PATTERNS> &equiv) {
        // The point is that eval(H0 u H1) >= eval(H0) + eval(H1)
        static int mm[] = {1,3,9,27,81};
        for (int j = 0; j < 5; j++){
            int s0 = s - ((s/mm[j])%3)*mm[j];
            int v = 0;
            for (int a = 0; a < 3; a++) v = std::min(v + lb[s0+mm[j]*a], INF_INT);
            auto cacheKey = getCacheKey(equiv[s0], guesses, remDepth-1);
            for (auto ind: equiv[s0+mm[j]]) cacheKey.state.wsAnswers[ind] = true;
            for (auto ind: equiv[s0+2*mm[j]]) cacheKey.state.wsAnswers[ind] = true;
            setLbCacheVal(cacheKey, v, guessIndex);
        }
    }

    void someLbHack2(const GuessesVec &guesses, const IndexType guessIndex, RemDepthType remDepth, int numWrong, PatternType s, const std::array<AnswersVec, NUM_PATTERNS> &equiv) {
        static int mm[] = {1,3,9,27,81};
        if (numWrong == 0) return;

        // The useful case to apply this to is "when a new letter scores 'B', at least it's better than wasting a letter (by reusing a known non-letter)"
        for(int i=0;i<5;i++)if( (s/mm[i])%3==0 && (equiv[s+mm[i]].size()>0||equiv[s+2*mm[i]].size()>0) ){
            auto cacheKey = getCacheKey(equiv[s], guesses, remDepth-1);
            for (auto ind: equiv[s + mm[i]]) cacheKey.state.wsAnswers[ind] = true;
            for (auto ind: equiv[s + 2*mm[i]]) cacheKey.state.wsAnswers[ind] = true;
            setLbCacheVal(cacheKey, numWrong, guessIndex);
        }
        for(int i=0;i<4;i++)if((s/mm[i])%3==0)for(int j=i+1;j<5;j++)if((s/mm[j])%3==0){
            int a,b,t=0;
            for(a=0;a<3;a++)for(b=0;b<3;b++)t+=equiv[s+a*mm[i]+b*mm[j]].size();

            if(t>int(equiv[s].size())){
                auto cacheKey = getCacheKey(AnswersVec(), guesses, remDepth-1);
                for(a=0;a<3;a++)for(b=0;b<3;b++){
                    auto &e=equiv[s+a*mm[i]+b*mm[j]];
                    for (auto ind: e) cacheKey.state.wsAnswers[ind] = true;
                }
                setLbCacheVal(cacheKey, numWrong, guessIndex);
                //writelboundcache(depth+1,filtered[s],u,inc-u.size());
            }
        }
        // for(int i=0;i<3;i++)if((s/mm[i])%3==0)for(int j=i+1;j<4;j++)if((s/mm[j])%3==0)for(int k = j+1; k<5; ++k)if((s/mm[k])%3==0){
        //     int a,b,c,t=0;
        //     for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c)t+=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]].size();

        //     if(t>int(equiv[s].size())){
        //         auto cacheKey = getCacheKey(AnswersVec(), guesses, remDepth-1);
        //         for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c){
        //             auto &e=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]];
        //             for (auto ind: e) cacheKey.state.wsAnswers[ind] = true;
        //         }
        //         setLbCacheVal(cacheKey, numWrong, guessIndex);
        //         //writelboundcache(depth+1,filtered[s],u,inc-u.size());
        //     }
        // }

        // for(int i=0;i<2;i++)if((s/mm[i])%3==0)for(int j=i+1;j<3;j++)if((s/mm[j])%3==0)for(int k = j+1; k<4; ++k)if((s/mm[k])%3==0)for(int l = k+1; l<5; ++l)if((s/mm[l])%3==0){
        //     int a,b,c,d,t=0;
        //     for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c)for(d=0;d<3;++d)t+=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]+d*mm[l]].size();

        //     if(t>int(equiv[s].size())){
        //         auto cacheKey = getCacheKey(AnswersVec(), guesses, remDepth-1);
        //         for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c)for(d=0;d<3;++d){
        //             auto &e=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]+d*mm[l]];
        //             for (auto ind: e) cacheKey.state.wsAnswers[ind] = true;
        //         }
        //         setLbCacheVal(cacheKey, numWrong, guessIndex);
        //         //writelboundcache(depth+1,filtered[s],u,inc-u.size());
        //     }
        // }
    }

    // BestWordResult getGuessForLeastWrong(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, int beta) {
    //     assertm(remDepth != 0, "no tries remaining");
        
    //     if (answers.size() == 0) return {0, 0};
    //     // assumes its sorted
    //     //if (remDepth >= answers.size()) return {0, answers[std::min(remDepth-1, static_cast<int>(answers.size())-1)]};

    //     if (remDepth == 1) { // we can't use info from last guess
    //         return {
    //             (int)answers.size()-1,
    //             *std::min_element(answers.begin(), answers.end())
    //         };
    //     }

    //     const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, remDepth);
    //     if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
    //         return cacheVal;
    //     }

    //     GuessesVec guessesCopy;
    //     GuessesVec& guessesToUse = remDepth == 2
    //         ? RemoveGuessesWithBetterGuessCache::getCachedGuessesVec(answers)
    //         : (guessesCopy = guesses/*RemoveGuessesWithBetterGuessCache::buildGuessesVecWithoutRemovingAnyAnswer(guesses, answers)*/);
    //     std::array<IndexType, NUM_PATTERNS> equiv;
    //     std::array<int64_t, MAX_NUM_GUESSES> sortVec;
    //     BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(answers, guessesToUse, equiv, sortVec);
    //     if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
    //         return setCacheVal(key, minNumWrongFor2);
    //     }

    //     const bool shouldSort = true;
    //     if (shouldSort) std::sort(guessesCopy.begin(), guessesCopy.end(), [&](IndexType a, IndexType b) { return sortVec[a] < sortVec[b]; });

    //     // full search for remDepth >= 3
    //     std::array<AnswersVec, NUM_PATTERNS> &myEquiv = myEquivCache[remDepth];
    //     BestWordResult res = getDefaultBestWordResult();
    //     res.wordIndex = answers[0];

    //     std::size_t numGuessIndexesToCheck = std::min(guessesCopy.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
    //     const int initBeta = beta;
    //     for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
    //         const auto possibleGuess = guessesCopy[myInd];
    //         int numWrongForThisGuess = 0;
            
    //         for (std::size_t i = 0; i < NUM_PATTERNS; ++i) myEquiv[i].resize(0);
    //         for (const auto answerIndex: answers) {
    //             const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, possibleGuess);
    //             myEquiv[patternInt].push_back(answerIndex);
    //         }

    //         for (std::size_t i = 0; i < NUM_PATTERNS-1; ++i) {
    //             if (myEquiv[i].size() == 0) continue;
    //             const auto pr = makeGuessAndRestoreAfter(myEquiv[i], guessesCopy, remDepth, initBeta - numWrongForThisGuess);
    //             const auto expNumWrongForSubtree = pr.numWrong;
    //             numWrongForThisGuess += expNumWrongForSubtree;
    //             if (numWrongForThisGuess >= beta) { numWrongForThisGuess = INF_INT; break; }
    //         }

    //         BestWordResult newRes = {numWrongForThisGuess, possibleGuess};
    //         if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
    //             res = newRes;
    //             beta = std::min(beta, res.numWrong);
    //             if (res.numWrong == 0) {
    //                 return setCacheVal(key, res);
    //             }
    //         }
    //     }

    //     return res.numWrong == INF_INT ? res : setCacheVal(key, res);
    // }

    int getDefaultBeta() {
        if constexpr (isGetLowestAverage) {
            return GlobalArgs.maxTotalGuesses + 1;
        } else {
            return GlobalArgs.maxWrong + 1;
        }
    }

    BestWordResult getGuessForLowestAverage(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, int beta) {
        assertm(remDepth != 0, "no tries remaining");
        return {0, 0};

        /*

        if (answers.size() == 0) return {0, 0};
        if (answers.size() == 1) return {1, answers[0]};
        if (remDepth == 1) { // we can't use info from last guess
            return {
                INF_INT,
                answers[0]
            };
        }

        const int nh = answers.size();
        if (2*nh-1 >= beta) {
            return {2*nh-1, answers[0]};
        }

        const auto [key, cacheVal] = getCacheKeyAndValue(answers, guesses, remDepth);
        if (cacheVal.wordIndex != MAX_INDEX_TYPE) {
            return cacheVal;
        }

        int guessesRemovedByClearGuesses = 0;
        
        // when there is >1 word remaining, we need at least 2 tries to definitely guess it
        std::size_t numGuessIndexesToCheck = std::min(guesses.size(), static_cast<std::size_t>(GlobalArgs.guessLimitPerNode));
        std::array<IndexType, NUM_PATTERNS> patternToRep;
        std::array<int, NUM_PATTERNS> equiv;
        IndexType good = MAX_INDEX_TYPE;

        for (const auto guessIndex: answers) {
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);
            int bad = 0;
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                bad += (c>=2);
            }
            if (bad == 0) {
                return setCacheVal(key, {2*nh-1, guessIndex});
            }
            if (bad == 1) good = guessIndex;
        }
        if (good != MAX_INDEX_TYPE && remDepth >= 3) {
            return setCacheVal(key, {2*nh, good});
        }

        for (const auto guessIndex: guesses) {
            auto &lbCacheEntry = lbCache3d[remDepth][guessIndex];
            std::array<int, NUM_PATTERNS> &count = equiv;
            count.fill(0);
            lbCacheEntry.fill(0);

            int s2 = 0, innerLb = 0;
            for (const auto answerIndex: p.answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                int c = (++count[patternInt]);
                auto lbVal = 2 - (c == 1); // Assumes remdepth>=3
                innerLb += lbVal;
                lbCacheEntry[patternInt] += patternInt == NUM_PATTERNS-1 ? 0 : lbVal;
                s2 += 2*c - 1;
            }
            innerLb -= count[NUM_PATTERNS - 1];
            if (count[NUM_PATTERNS - 1] == 0 && s2 == nh) {
                p.guesses.restoreValues(guessesRemovedByClearGuesses);
                return setCacheVal(key, {2*nh, guessIndex});
            }
            auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
            p.guesses.registerSortVec(guessIndex, sortVal);
        }

        // no split into singletons --> infinity
        if (remDepth <= 2) {
            return setCacheVal(key, {INF_INT, p.answers[0]});
        }

        guesses.sortBySortVec();

        // begin exact for remDepth >= 3
        BestWordResult res = getDefaultBestWordResult();
        res.wordIndex = answers[0];

        for (std::size_t myInd = 0; myInd < numGuessIndexesToCheck; myInd++) {
            const auto possibleGuess = guesses[myInd];
            const auto &lbCacheEntry = lbCache3d[remDepth][possibleGuess];

            equiv.fill(0);
            int lbCacheSum = nh;

            for (const auto actualWordIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                equiv[patternInt]++;
                if (equiv[patternInt] == 1) {
                    lbCacheSum += lbCacheEntry[patternInt];
                    patternToRep[patternInt] = actualWordIndex;
                }
            }

            for (auto actualWordIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualWordIndex, possibleGuess);
                if (patternToRep[patternInt] != actualWordIndex) continue;

                if (lbCacheSum >= beta) { lbCacheSum = INF_INT; break; }

                lbCacheSum -= lbCacheEntry[patternInt];

                const auto bestLb = std::max(0, lbCacheSum);
                const auto pr = makeGuessAndRestoreAfter(p, possibleGuess, actualWordIndex, remDepth, beta - bestLb);

                const auto numWrong = pr.numWrong;

                if (numWrong == INF_INT) { lbCacheSum = INF_INT;  break; }
                lbCacheSum += numWrong;
            }

            BestWordResult newRes = {lbCacheSum, possibleGuess};
            if (newRes.numWrong < res.numWrong || (newRes.numWrong == res.numWrong && newRes.wordIndex < res.wordIndex)) {
                res = newRes;
                beta = std::min(beta, res.numWrong);
            }
        }

        return res.numWrong == INF_INT ? res : setCacheVal(key, res);*/
    }

    inline BestWordResult makeGuessAndRestoreAfter(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        const int beta)
    {
        if constexpr (isEasyMode) {
            return getGuessFunctionDecider(answers, guesses, remDepth-1, beta);
        } else {
            //auto guesses2 = guesses; // todo
           return getGuessFunctionDecider(answers, guesses, remDepth-1, beta);
        }
    }


    inline BestWordResult setCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        if (res.numWrong != 0) setLbCacheVal(key, res.numWrong);
        return getGuessCache[key] = res;
    }

    std::pair<AnswersAndGuessesKey<isEasyMode>, const BestWordResult> getCacheKeyAndValue(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth) {
        auto key = getCacheKey(answers, guesses, remDepth);
        return {key, getCache(key)};
    }

    BestWordResult getCache(const AnswersAndGuessesKey<isEasyMode> &key) {
        stats.tick(4446);
        auto it = getGuessCache.find(key);
        if (it == getGuessCache.end()) {
            cacheMiss++; stats.tock(4446);
            return getDefaultBestWordResult();
        }
        cacheHit++; stats.tock(4446);
        return it->second;
    }

    int getLbCache(const AnswersAndGuessesKey<isEasyMode> &key) {
        stats.tick(4447);
        auto it = lbGuessCache.find(key);
        if (it == lbGuessCache.end()) {
            lbCacheMiss++; stats.tock(4447);
            return 0;
        }
        lbCacheHit++; stats.tock(4447);
        return it->second;
    }   

    inline static int potential = 0, realised = 0;
    void setLbCacheVal(const AnswersAndGuessesKey<isEasyMode> &key, int v, IndexType guessIndex = MAX_INDEX_TYPE) {
        if (v == 0) return;
        if (v < 0) {
            DEBUG("OK, WHAT. given: " << v);
            exit(1);
        }
        if (v >= GlobalArgs.maxWrong) {
            realised += guessIndex != MAX_INDEX_TYPE;
            //DEBUG("POTENTIAL" << (++potential) << " / " << realised);
            if (guessIndex != MAX_INDEX_TYPE) {
                setCacheVal(key, {v, guessIndex});
            }
        }
        auto r = lbGuessCache.try_emplace(key, v);
        if (!r.second) {
            if (v > r.first->second) {
                lbGuessCache.emplace(key, v);
            }
        }
    }

    AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswersVec &answers, const GuessesVec &guesses, RemDepthType remDepth) {
        //stats.tick(4448);
        auto res = getCacheKeyInner(answers, guesses, remDepth);
        //stats.tock(4448);
        return res;
    }   

    AnswersAndGuessesKey<isEasyMode> getCacheKeyInner(const AnswersVec &answers, const GuessesVec &guesses, RemDepthType remDepth) {
        if constexpr (isEasyMode) {
            return AnswersAndGuessesKey<isEasyMode>(answers, remDepth);
        } else {
            return AnswersAndGuessesKey<isEasyMode>(answers, guesses, remDepth);
        }
    }

    static const BestWordResult& getDefaultBestWordResult() {
        static BestWordResult defaultBestWordResult = {INF_INT, MAX_INDEX_TYPE};
        return defaultBestWordResult;
    }
};
