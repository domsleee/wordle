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
#include "RemoveGuessesPartitions.h"

#include "EndGameAnalysis.h"
#include "PerfStats.h"
#include "NonLetterLookup.h"
#include "LookupCacheEntry.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <bitset>
#include <set>
#include <stack>
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) 

using namespace NonLetterLookupHelpers;

template <bool isEasyMode>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(RemDepthType maxTries)
        : maxTries(maxTries)
        {}

    const RemDepthType maxTries;
    std::string startingWord = "";
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, LookupCacheEntry> guessCache = {};
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
            auto firstPr = minOverWordsLeastWrong(answers, guesses, maxTries, 0, getDefaultBeta());
            DEBUG("first word: " << GlobalState.reverseIndexLookup[firstPr.wordIndex] << ", known numWrong: " << firstPr.numWrong);
            firstWordIndex = firstPr.wordIndex;
        } else {
            firstWordIndex = GlobalState.getIndexForWord(startingWord);
        }
        auto answerIndex = GlobalState.getIndexForWord(answer);
        return solveWord(answerIndex, solutionModel, firstWordIndex, answers, guesses);
    }

    AnswersAndGuessesResult solveWord(IndexType answerIndex, std::shared_ptr<SolutionModel> solutionModel, IndexType firstWordIndex, AnswersVec &answers, GuessesVec &guesses) {
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

            auto pr = minOverWordsLeastWrong(answers, guesses, maxTries-res.tries, 0, getDefaultBeta());
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
        if (remDepth == 2) {
            return calcSortVectorAndGetMinNumWrongFor2RemDepth2(answers, guesses, beta);
        }
        BestWordResult minNumWrongFor2 = {INF_INT, 0};
        auto &count = equiv;
        int nh = answers.size();

        uint8_t thr = remDepth + (remDepth >= 3 ? (anyNSolvedIn2Guesses - 2) : 0);
        assert(thr >= 3);

        for (const auto guessIndex: guesses) {
            // const auto [numWrongFor2, sortVal] = calcSortVectorAndGetMinNumWrongFor2(guessIndex, answers, count, remDepth);
            int innerLb = 0, s2 = 0;
            bool maxCLtThr = true; // maxC < thr
            count.fill(0);
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                auto &c = count[patternInt];
                ++c;
                innerLb += 2 - (c <= 1); // Assumes remdepth>=3
                s2 += 2*c - 1;
                if (c >= thr) { maxCLtThr = false; }
            }
            
            if (maxCLtThr) {
                return {0, guessIndex};
            }
            innerLb -= count[NUM_PATTERNS - 1];
            auto sortVal = (int64_t(2*s2 + nh*innerLb)<<32)|guessIndex;
            sortVec[guessIndex] = sortVal;
        }

        return minNumWrongFor2;
    }

    BestWordResult calcSortVectorAndGetMinNumWrongFor2RemDepth2(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        int beta) const
    {
        BestWordResult minNumWrongFor2 = {INF_INT, 0};
        std::array<uint8_t, 243> seen;
        for (const auto guessIndex: guesses) {
            seen.fill(0);//std::fill(seen.begin() + mVal, seen.begin() + MVal, 0);
            int numWrongFor2 = 0;
            for (const auto answerIndex: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(answerIndex, guessIndex);
                auto &seenVal = seen[patternInt];
                numWrongFor2 += seenVal;
                if (numWrongFor2 >= minNumWrongFor2.numWrong) break; // it seems slower to use beta and get lbs here.
                seenVal = 1;
            }

            if (numWrongFor2 < minNumWrongFor2.numWrong) {
                minNumWrongFor2 = {numWrongFor2, guessIndex};
                if (numWrongFor2 < beta) {
                    beta = numWrongFor2;
                }
            }
            
            if (numWrongFor2 == 0) {
                return {0, guessIndex};
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

    //const bool canAny3BeSolvedIn2 = true;
    const int anyNSolvedIn2Guesses = 3; // not 4: batty,patty,tatty,fatty
    BestWordResult minOverWordsLeastWrong(const AnswersVec &answers, const GuessesVec &guesses, const RemDepthType remDepth, FastModeType fastMode, int beta) {
        assertm(remDepth != 0, "no tries remaining");
        stats.entryStats[GlobalArgs.maxTries-remDepth][0]++;
        stats.nodes++;

        if (answers.size() == 0) return {0, 0};
        if (answers.size() == 1) return {0, answers[0]};
        const int nAnswers = answers.size();
        if (remDepth + (remDepth >= 3 ? (anyNSolvedIn2Guesses - 2) : 0) >= nAnswers) {
            //auto m = *std::min_element(answers.begin(), answers.end());
            assert(answers[0] == *std::min_element(answers.begin(), answers.end()));
            return {0, answers[0]};
        }
        if (remDepth == 1) {
            // no info from last guess
            //auto m = *std::min_element(answers.begin(), answers.end());
            assert(answers[0] == *std::min_element(answers.begin(), answers.end()));
            return {nAnswers-1, answers[0]};
        }

        stats.entryStats[GlobalArgs.maxTries-remDepth][1]++;
        if (fastMode == 1) return {-1, 0};

        const auto key = getCacheKey(answers, guesses, remDepth);
        const auto cacheVal = getCache(key);
        if (cacheVal.ub != -1 && (cacheVal.lb == cacheVal.ub || cacheVal.lb >= beta)) {
            return BestWordResult(cacheVal.lb, cacheVal.guessForLb);
        }
        stats.entryStats[GlobalArgs.maxTries-remDepth][2]++;

        maybePruneCache();

        std::array<uint8_t, NUM_PATTERNS> count;
        stats.tick(45);
        //auto perfectAnswerCandidates = answers;
        //auto nonLetterMaskNoSpecialMask2 = RemoveGuessesWithBetterGuessCache::getNonLetterMaskNoSpecialMask(answers);

        uint8_t thr = remDepth + (remDepth >= 3 ? (anyNSolvedIn2Guesses - 2) : 0);
        for (const auto answerIndexForGuess: answers) {
            count.fill(0);
            bool maxCBelowThr = true;
            for (const auto actualAnswer: answers) {
                const auto patternInt = PatternGetterCached::getPatternIntCached(actualAnswer, answerIndexForGuess);
                auto c = ++count[patternInt];
                if (c >= thr) { maxCBelowThr = false; break; }
            }
            if (maxCBelowThr) {
                stats.tock(45);
                BestWordResult res = {0, answerIndexForGuess};
                setOptCache(key, res);
                return res;
            }
        }
        stats.tock(45);

        if (fastMode == 2) return {-1, 0};

        std::array<int64_t, MAX_NUM_GUESSES> sortVec;
        const int depth = GlobalArgs.maxTries - remDepth;

        stats.tick(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));
        auto nonLetterMaskNoSpecialMask = RemoveGuessesWithBetterGuessCache::getNonLetterMaskNoSpecialMask(answers);
        auto nonLetterMask = nonLetterMaskNoSpecialMask & RemoveGuessesWithNoLetterInAnswers::specialMask;

        auto &guessesDisregardingAnswers = RemoveGuessesWithBetterGuessCache::cache[nonLetterMask];
        stats.tock(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));

        stats.tick(50 + depth);
        auto &myGuesses = guessesDisregardingAnswers.size() < guesses.size() ? guessesDisregardingAnswers : guesses;
        std::array<IndexType, NUM_PATTERNS> equiv;
        BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(answers, myGuesses, equiv, sortVec, remDepth, beta);
        stats.tock(50 + depth);
        if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
            bool exact = true;// remDepth > 2 || minNumWrongFor2.numWrong < beta;
            if (exact) setOptCache(key, minNumWrongFor2);
            else setLbCache(key, minNumWrongFor2);
            return minNumWrongFor2;
        }

        stats.entryStats[depth][3]++;

        GuessesVec guessesCopy = myGuesses;
        // auto bef = guessesCopy.size();
        stats.tick(33);
        stats.tick(10);
        char greenLetters[5];
        uint8_t maxLetterCt[27] = {0}, greenLetterCt[27] = {0};
        for (auto i = 0; i < 5; ++i) greenLetters[i] = GlobalState.reverseIndexLookup[answers[0]][i];
        for (auto answerIndex: answers) {
            uint8_t letterCt[27] = {0};
            auto &answerWord = GlobalState.reverseIndexLookup[answerIndex];
            for (int i = 0; i < 5; ++i) {
                if (greenLetters[i] != answerWord[i]) greenLetters[i] = '.';
                letterCt[answerWord[i]-'a']++;
            }
            for (int i = 0; i < 5; ++i) {
                maxLetterCt[answerWord[i]-'a'] = std::max(maxLetterCt[answerWord[i]-'a'], letterCt[answerWord[i]-'a']);
            }
        }
        for (int i = 0; i < 5; ++i) {
            greenLetterCt[letterToInd(greenLetters[i])]++;
        }
        for (int i = 0; i < 5; ++i) {
            if (greenLetterCt[letterToInd(greenLetters[i])] < maxLetterCt[letterToInd(greenLetters[i])]) {
                greenLetters[i] = '.'; // we must be careful! if we don't know where all of the green letter is, we can't exclude it
            }
        }
        for (int i = 0; i < 5; ++i) {
            if (greenLetters[i] != '.') {
                // DEBUG("JUICED " << greenLetters[i] << " help? " << getBoolVal((1 << (greenLetters[i] - 'a') & nonLetterMaskNoSpecialMask) == 0));
                nonLetterMaskNoSpecialMask |= (1 << (greenLetters[i] - 'a'));
            }
        }

        stats.tock(10);
        RemoveGuessesWithNoLetterInAnswers::removeWithBetterOrSameGuessFaster(stats, guessesCopy, nonLetterMaskNoSpecialMask); // removes a few more
        stats.tock(33);
        // auto numRemoved = bef - guessesCopy.size();
        // DEBUG("#removed: " << numRemoved);
        std::sort(guessesCopy.begin(), guessesCopy.end(), [&](IndexType a, IndexType b) { return sortVec[a] < sortVec[b]; });

        // full search for remDepth >= 3
        BestWordResult res = getDefaultBestWordResult();
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

        if (exact) setOptCache(key, res);
        else setLbCache(key, res);
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
        stats.tick(20);
        int totalWrongForGuess = 0;
        for (PatternType s = 0; s < NUM_PATTERNS-1; ++s) {
            std::size_t sz = myEquiv[s].size();
            if (sz == 0) continue;
            if (sz == answers.size()) return INF_INT;
            auto innerRes = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 1, beta - totalWrongForGuess);
            if (innerRes.numWrong != -1) {
                lb[s] = innerRes.numWrong;
            } else {
                lb[s] = 0;
                indexToPattern[n++] = s;
            }

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) { stats.tock(20); return totalWrongForGuess; }
        }
        stats.tock(20);

        stats.tick(21);
        int m = 0;
        for (int i = 0; i < n; ++i) {
            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            auto innerRes = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 2, beta - totalWrongForGuess);
            if (innerRes.numWrong != -1) {
                lb[s] = innerRes.numWrong;
            } else {
                if (remDepth-1 >= 2) {
                    auto lbKey = getCacheKey(myEquiv[s], guesses, remDepth-1);
                    auto cacheEntry = getCache(lbKey);
                    lb[s] = cacheEntry.lb;
                    if (cacheEntry.ub != cacheEntry.lb) {
                        indexToPattern[m++] = s;
                    }
                } else {
                    lb[s] = 0;
                    indexToPattern[m++] = s;
                }
            }

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) { stats.tock(21); return totalWrongForGuess; }
        }
        stats.tock(21);

        n = m;

        for (int i = 0; i < n; ++i) {
            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            auto lbKey = getCacheKey(myEquiv[s], guesses, remDepth-1);
            BestWordResult endGameRes = endGameAnalysisCached(myEquiv[s], guesses, lbKey, remDepth, beta);
            lb[s] = std::max(lb[s], endGameRes.numWrong);

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) {
                return totalWrongForGuess;
            }
        }

        stats.tick(22);
        int score[243];
        for (int i = 0; i < n; ++i) {
            int s = indexToPattern[i];
            score[s] = myEquiv[s].size();
        }
        std::sort(indexToPattern, indexToPattern+n, [&score](auto s0, auto s1) -> bool { return score[s0] < score[s1];});
        stats.tock(22);

        auto depth = GlobalArgs.maxTries - remDepth;
        stats.tick(23+depth);
        for (int i = 0; i < n; ++i) {
            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            const auto pr = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 0, beta - totalWrongForGuess);
            totalWrongForGuess = std::min(totalWrongForGuess + pr.numWrong, INF_INT);
            if (totalWrongForGuess >= beta) {
                stats.tick(34);
                someLbHack(guesses, guessIndex, remDepth, lb, s, myEquiv);
                someLbHack2(guesses, guessIndex, remDepth, totalWrongForGuess, s, myEquiv);

                stats.tock(34);
                stats.tock(23+depth);
                return totalWrongForGuess;
            }
        }

        stats.tock(23+depth);
        return totalWrongForGuess;
    }

    BestWordResult endGameAnalysisCached(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const AnswersAndGuessesKey<isEasyMode> &cacheKey,
        RemDepthType remDepth,
        int beta
    )
    {
        std::vector<int> endGameMasks(EndGameAnalysis::numEndGames, 0);
        for (auto answerIndex: answers) for (auto [e, maskIndex]: EndGameAnalysis::wordNum2EndGamePair[answerIndex]) {
            endGameMasks[e] |= (1 << maskIndex);
        }
        auto maxR = BestWordResult(-1, 0);
        for (int e = 0; e < EndGameAnalysis::numEndGames; ++e) {
            auto r = EndGameAnalysis::getFromCache(remDepth, e, endGameMasks[e]);
            if (r.numWrong > maxR.numWrong) {
                maxR = r;
            }
        }
        if (maxR.numWrong >= beta) {
            setLbCache(cacheKey, maxR);
            return maxR;
        }
        return {0, answers[0]};

    }

    void someLbHack(const GuessesVec &guesses, const IndexType guessIndex, RemDepthType remDepth, const int *lb, PatternType s, const std::array<AnswersVec, NUM_PATTERNS> &equiv) {
        // The point is that eval(H0 u H1) >= eval(H0) + eval(H1)
        static int mm[] = {1,3,9,27,81};
        for (int j = 0; j < 5; j++){
            int s0 = s - ((s/mm[j])%3)*mm[j];
            int v = 0;
            for (int a = 0; a < 3; a++) v = std::min(v + lb[s0+mm[j]*a], INF_INT);
            if (v == 0) continue;
            auto cacheKey = getCacheKey(equiv[s0], guesses, remDepth-1);
            for (auto ind: equiv[s0+mm[j]]) cacheKey.state.wsAnswers[ind] = true;
            for (auto ind: equiv[s0+2*mm[j]]) cacheKey.state.wsAnswers[ind] = true;
            setLbCache(cacheKey, {v, guessIndex});
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
            setLbCache(cacheKey, {numWrong, guessIndex});
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
                setLbCache(cacheKey, {numWrong, guessIndex});
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
        //         setLbCache(cacheKey, {numWrong, guessIndex});
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
        //         setLbCache(cacheKey, {numWrong, guessIndex});
        //     }
        // }
    }

    int getDefaultBeta() {
        return GlobalArgs.maxWrong + 1;
    }

    inline void setOptCache(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        setCacheEntry(key, res.numWrong, res.numWrong, res.wordIndex, res.wordIndex);
    }

    inline void setLbCache(const AnswersAndGuessesKey<isEasyMode> &key, BestWordResult res) {
        if (res.numWrong == 0) return;
        if (key.state.remDepth < GlobalArgs.minLbCache) return;
        setCacheEntry(key, res.numWrong, INF_INT, res.wordIndex, 0);
    }

    void setCacheEntry(const AnswersAndGuessesKey<isEasyMode> &key, int lb, int ub, IndexType guessForLb, IndexType guessForUb) {
        auto it = guessCache.find(key);
        if (it != guessCache.end()) {
            auto &curr = it->second;
            if (lb > curr.lb) { curr.lb = lb; curr.guessForLb = guessForLb; }
            if (ub < curr.ub) { curr.ub = ub; curr.guessForUb = guessForUb; }
        } else {
            stats.addCacheWrite(key.state.remDepth);
            static std::ofstream cacheDebug("cacheDebug");
            cacheDebug << key.state.wsAnswers.count() << '\n';
            cacheDebug.flush();
            guessCache[key] = LookupCacheEntry(guessForLb, guessForUb, lb, ub);
        }
    }

    std::pair<AnswersAndGuessesKey<isEasyMode>, LookupCacheEntry> getCacheKeyAndValue(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth) {
        auto key = getCacheKey(answers, guesses, remDepth);
        return {key, getCache(key)};
    }

    LookupCacheEntry getCache(const AnswersAndGuessesKey<isEasyMode> &key) {
        stats.tick(46);
        auto it = guessCache.find(key);
        if (it == guessCache.end()) {
            stats.addCacheMiss(key.state.remDepth);
            stats.tock(46);
            return LookupCacheEntry(0, 0, 0, -1);
        }
        stats.addCacheHit(key.state.remDepth);
        stats.tock(46);
        return it->second;
    }

    AnswersAndGuessesKey<isEasyMode> getCacheKey(const AnswersVec &answers, const GuessesVec &guesses, RemDepthType remDepth) {
        stats.tick(48);
        auto res = getCacheKeyInner(answers, guesses, remDepth);
        stats.tock(48);
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

    long long nextCheck = 0;
    static constexpr std::size_t keySize = sizeof(AnswersAndGuessesKey<isEasyMode>);
    static constexpr std::size_t valSize = sizeof(LookupCacheEntry);

    void maybePruneCache() {
        if (stats.nodes >= nextCheck) {
            // each cache entry = 
            auto entrySize = (sizeof(void*) + keySize + valSize);
            // DEBUG("entrySize: " << entrySize);
            // DEBUG("sizeof(void*): " << sizeof(void*));
            // DEBUG("keySize: " << keySize);
            // DEBUG("valSize: " << valSize);
            auto approxBytes = 1.1 * guessCache.size() * entrySize;
            if (approxBytes >= GlobalArgs.memLimitPerThread * 1e9) {
                DEBUG("pruned ;)");
                guessCache.clear();
            }
        }
    }
};
