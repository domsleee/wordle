#pragma once
#include "AnswersAndGuessesKey.h"
#include "AnswersAndGuessesResult.h"
#include "AttemptState.h"
#include "AttemptStateFaster.h"
#include "BestWordResult.h"
#include "Defs.h"
#include "EndGameAnalysis/EndGameAnalysis.h"
#include "GlobalState.h"
#include "LookupCacheEntry.h"
#include "PatternGetter.h"
#include "PerfStats.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesPartitions.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesPartitionsEqualOnly.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesPartitionsTrie.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesPartitionsPairSeq.h"
#include "RemoveGuessesBetterGuess/RemoveGuessesWithBetterGuessCache.h"
#include "SubsetCache/BiTrieSubsetCache.h"
#include "SubsetCache/SubsetCache.h"
#include "UnorderedVector.h"
#include "WordSetUtil.h"

#include <algorithm>
#include <bitset>
#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#define GUESSESSOLVER_DEBUG(x) 

using namespace NonLetterLookupHelpers;

template <bool isEasyMode>
struct AnswersAndGuessesSolver {
    AnswersAndGuessesSolver(RemDepthType maxTries)
        : maxTries(maxTries),
          subsetCache(SubsetCache(maxTries))
        {
            guessCache2.resize(maxTries+1);
            cacheSize.assign(maxTries+1, 0);
            for (int i = 0; i <= maxTries; ++i) {
                biTrieSubsetCaches.push_back(BiTrieSubsetCache(i));
            }
        }

    const RemDepthType maxTries;
    std::string startingWord = "";
    std::vector<std::map<std::pair<AnswersVec, GuessesVec>, LookupCacheEntry>> guessCache2;
    std::vector<uint64_t> cacheSize = {};
    std::unordered_map<AnswersAndGuessesKey<isEasyMode>, LookupCacheEntry> guessCache = {};
    PerfStats stats;
    SubsetCache subsetCache;
    std::vector<BiTrieSubsetCache> biTrieSubsetCaches;

    bool disableEndGameAnalysis = false;

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
        assert(beta != 0);
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
                    if (numWrongFor2 == 0) return {0, guessIndex};
                }
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
    static const int anyNSolvedIn2Guesses = 3; // not 4: batty,patty,tatty,fatty
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

        const auto cacheVal = getCache(answers, guesses, remDepth);
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
                setOptCache(answers, guesses, remDepth, res);
                return res;
            }
        }
        stats.tock(45);

        if (fastMode == 2) return {-1, 0};

        std::array<int64_t, MAX_NUM_GUESSES> sortVec;
        const int depth = GlobalArgs.maxTries - remDepth;

        stats.tick(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));
        auto nonLetterMaskNoSpecialMask = RemoveGuessesWithBetterGuessCache::getNonLetterMaskNoSpecialMask(answers);
        stats.tick(35);
        nonLetterMaskNoSpecialMask |= getGreenLetterMask(answers);
        stats.tock(35);
        auto nonLetterMask = nonLetterMaskNoSpecialMask & RemoveGuessesUsingNonLetterMask::specialMask;

        auto &guessesDisregardingAnswers = readGuessesWithBetterGuessCache(nonLetterMask);
        stats.tock(TIMING_DEPTH_REMOVE_GUESSES_BETTER_GUESS(depth));

        stats.tick(50 + depth);
        auto &myGuesses = guessesDisregardingAnswers.size() < guesses.size() ? guessesDisregardingAnswers : guesses;
        std::array<IndexType, NUM_PATTERNS> equiv;
        BestWordResult minNumWrongFor2 = calcSortVectorAndGetMinNumWrongFor2(answers, myGuesses, equiv, sortVec, remDepth, beta);
        stats.tock(50 + depth);
        if (minNumWrongFor2.numWrong == 0 || remDepth <= 2) {
            bool exact = true;// remDepth > 2 || minNumWrongFor2.numWrong < beta;
            if (exact) setOptCache(answers, guesses, remDepth, minNumWrongFor2);
            else setLbCache(answers, guesses, remDepth, minNumWrongFor2);
            return minNumWrongFor2;
        }

        assert(remDepth >= 3);
        stats.entryStats[depth][3]++;

        GuessesVec guessesCopy = myGuesses;
        // auto bef = guessesCopy.size();
        const int T = guessesCopy.size(), H = answers.size();
        if (true && depth == 2 && (true || T*H < 300000)) {
            // O(T^2.H)
            stats.tick(32);
            //DEBUG("H: " << H << ", T: " << T);
            //RemoveGuessesUsingNonLetterMask::removeWithBetterOrSameGuessFaster(stats, guessesCopy, nonLetterMaskNoSpecialMask); // removes a few more
            removeWithBetterOrSameGuessPartitions(guessesCopy, answers, PartitionStrategy::useOldVersion);
            stats.tock(32);
        }
        else if (GlobalArgs.usePartitions && (3 <= remDepth && remDepth <= 4)) {
            stats.tick(32);
            removeWithBetterOrSameGuessPartitions(guessesCopy, answers, PartitionStrategy::useNewVersion);
            stats.tock(32);
        } else {
            // O(TlgT)
            stats.tick(33);
            const auto yellowLetterMask = RemoveGuessesUsingNonLetterMask::getYellowNonLetterMasks(answers);
            RemoveGuessesUsingNonLetterMask(stats, nonLetterMaskNoSpecialMask, yellowLetterMask).removeWithBetterOrSameGuessFaster(guessesCopy); // removes a few more
            stats.tock(33);
        }
        if (depth-1 <= GlobalArgs.printLength) { prs((depth-1)*INDENT); printf("T%dc %9.2f %ld\n", depth-1, PerfStats::cpu(), guessesCopy.size()); }
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
            if(depth <= GlobalArgs.printLength){prs(depth*INDENT);printf("M%d %ld/%ld\n", depth, myInd, numGuessIndexesToCheck);}
            auto r = sumOverPartitionsLeastWrong(answers, guessesCopy, remDepth-1, possibleGuess, beta);
            if(depth <= GlobalArgs.printLength){prs(depth*INDENT);printf("N%d %ld/%ld\n", depth, myInd, numGuessIndexesToCheck);}
            if (r < res.numWrong) {
                res = {r, possibleGuess};
                if (r < beta) { beta = r; exact = true; }
            }
            if (r == 0) break;
        }

        if (exact) setOptCache(answers, guesses, remDepth, res);
        else setLbCache(answers, guesses, remDepth, res);
        return res;
    }

    const auto &readGuessesWithBetterGuessCache(int nonLetterMask) {
        return RemoveGuessesWithBetterGuessCache::cache[nonLetterMask];
    }

    enum PartitionStrategy {
        compareWithSlower = 0,
        useOldVersion,
        useNewVersion,
        useEqualOnly,
        usePairSeq
    };
    void removeWithBetterOrSameGuessPartitions(GuessesVec &guesses, const AnswersVec &answers, PartitionStrategy strategy) {
        if (strategy == PartitionStrategy::usePairSeq) {
            RemoveGuessesPartitionsPairSeq::removeWithBetterOrSameGuess(stats, guesses, answers);
        }
        else if (strategy == PartitionStrategy::useEqualOnly) {
            RemoveGuessesPartitionsEqualOnly::removeWithBetterOrSameGuess(stats, guesses, answers);
        }
        else if (strategy == PartitionStrategy::useNewVersion) {
            RemoveGuessesPartitionsTrie::removeWithBetterOrSameGuess(stats, guesses, answers);
        } else if (strategy == PartitionStrategy::useOldVersion) {
            RemoveGuessesPartitions::removeWithBetterOrSameGuess(stats, guesses, answers);
        }
        else if (strategy == PartitionStrategy::compareWithSlower) {
            auto orig = guesses;
            auto slower = guesses;
            RemoveGuessesPartitions::removeWithBetterOrSameGuess(stats, slower, answers);
            RemoveGuessesPartitionsTrie::removeWithBetterOrSameGuess(stats, guesses, answers);
            if (guesses != slower) {
                DEBUG("DIFFERENT!@");
                DEBUG("orig   : " << getIterableString(orig));
                DEBUG("slower : " << getIterableString(slower));
                DEBUG("trie   : " << getIterableString(guesses));
                exit(1);
            }
        }
    }

    int sumOverPartitionsLeastWrong(
        const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth, // remDepth for each partition
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
                assert(remDepth >= 2);
                auto cacheEntry = getCache(myEquiv[s], guesses, remDepth);
                lb[s] = cacheEntry.lb;
                if (cacheEntry.ub != cacheEntry.lb) {
                    indexToPattern[m++] = s;
                }
            }

            totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
            if (totalWrongForGuess >= beta) { stats.tock(21); return totalWrongForGuess; }
        }
        stats.tock(21);

        n = m;
        if (n == 0) return totalWrongForGuess;

        assert(remDepth >= 2);
        if (!disableEndGameAnalysis) {
            for (int i = 0; i < n; ++i) {
                auto s = indexToPattern[i];
                totalWrongForGuess -= lb[s];
                stats.tick(41);
                BestWordResult endGameRes = endGameAnalysisCached(myEquiv[s], guesses, remDepth, beta);
                stats.tock(41);
                lb[s] = std::max(lb[s], endGameRes.numWrong);
                
                int lb2 = getLbFromAnswerSubsets(myEquiv[s], guesses, remDepth);
                lb[s] = std::max(lb[s], lb2);

                totalWrongForGuess = std::min(totalWrongForGuess + lb[s], INF_INT);
                if (totalWrongForGuess >= beta) {
                    return totalWrongForGuess;
                }
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
            if (depth-1 <= GlobalArgs.printLength) { prs((depth-1)*INDENT); printf("S%dc %9.2f %d/%d\n", depth-1, PerfStats::cpu(), i, n); }

            auto s = indexToPattern[i];
            totalWrongForGuess -= lb[s];
            const auto pr = minOverWordsLeastWrong(myEquiv[s], guesses, remDepth, 0, beta - totalWrongForGuess);
            totalWrongForGuess = std::min(totalWrongForGuess + pr.numWrong, INF_INT);
            lb[s] = std::max(lb[s], pr.numWrong);
            if (totalWrongForGuess >= beta) {
                stats.tick(34);
                someLbHack(guesses, guessIndex, remDepth, lb, s, myEquiv);
                someLbHack2(guesses, guessIndex, remDepth, lb[s], s, myEquiv);

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
        RemDepthType remDepth,
        int beta
    )
    {
        std::vector<int> endGameMasks(EndGameAnalysis::endGames.size(), 0);
        for (auto answerIndex: answers) {
            for (auto [e, maskIndex]: EndGameAnalysis::wordNum2EndGamePair[answerIndex]) {
                endGameMasks[e] |= (1 << maskIndex);
            }
        }
        auto maxR = BestWordResult(-1, 0);
        for (std::size_t e = 0; e < EndGameAnalysis::endGames.size(); ++e) {
            auto r = EndGameAnalysis::getFromCache(remDepth, e, endGameMasks[e]);
            if (r.numWrong > maxR.numWrong) {
                maxR = r;
            }
        }
        if (maxR.numWrong >= beta) {
            setLbCache(answers, guesses, remDepth, maxR, true);
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
            AnswersVec newAnswers = equiv[s0];
            for (auto ind: equiv[s0+mm[j]]) newAnswers.push_back(ind);
            for (auto ind: equiv[s0+2*mm[j]]) newAnswers.push_back(ind);
            std::sort(newAnswers.begin(), newAnswers.end());
            setLbCache(newAnswers, guesses, remDepth, {v, guessIndex});
        }
    }

    void someLbHack2(const GuessesVec &guesses, const IndexType guessIndex, RemDepthType remDepth, int numWrong, PatternType s, const std::array<AnswersVec, NUM_PATTERNS> &equiv) {
        static int mm[] = {1,3,9,27,81};
        if (numWrong == 0) return;

        // The useful case to apply this to is "when a new letter scores 'B', at least it's better than wasting a letter (by reusing a known non-letter)"
        for(int i=0;i<5;i++)if( (s/mm[i])%3==0 && (equiv[s+mm[i]].size()>0||equiv[s+2*mm[i]].size()>0) ){
            AnswersVec newAnswers = equiv[s];
            for (auto ind: equiv[s + mm[i]]) newAnswers.push_back(ind);
            for (auto ind: equiv[s + 2*mm[i]]) newAnswers.push_back(ind);
            std::sort(newAnswers.begin(), newAnswers.end());
            setLbCache(newAnswers, guesses, remDepth, {numWrong, guessIndex});
        }
        for(int i=0;i<4;i++)if((s/mm[i])%3==0)for(int j=i+1;j<5;j++)if((s/mm[j])%3==0){
            int a,b,t=0;
            for(a=0;a<3;a++)for(b=0;b<3;b++)t+=equiv[s+a*mm[i]+b*mm[j]].size();

            if(t>int(equiv[s].size())){
                AnswersVec newAnswers = {};
                for(a=0;a<3;a++)for(b=0;b<3;b++){
                    auto &e=equiv[s+a*mm[i]+b*mm[j]];
                    for (auto ind: e) newAnswers.push_back(ind);
                }
                std::sort(newAnswers.begin(), newAnswers.end());
                setLbCache(newAnswers, guesses, remDepth, {numWrong, guessIndex});
                //writelboundcache(depth+1,filtered[s],u,inc-u.size());
            }
        }
        for(int i=0;i<3;i++)if((s/mm[i])%3==0)for(int j=i+1;j<4;j++)if((s/mm[j])%3==0)for(int k = j+1; k<5; ++k)if((s/mm[k])%3==0){
            int a,b,c,t=0;
            for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c)t+=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]].size();

            if(t>int(equiv[s].size())){
                AnswersVec newAnswers = {};
                for(a=0;a<3;a++)for(b=0;b<3;b++)for(c=0;c<3;++c){
                    auto &e=equiv[s+a*mm[i]+b*mm[j]+c*mm[k]];
                    for (auto ind: e) newAnswers.push_back(ind);
                }
                std::sort(newAnswers.begin(), newAnswers.end());
                setLbCache(newAnswers, guesses, remDepth, {numWrong, guessIndex});
            }
        }

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

    inline void setOptCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth, BestWordResult res) {
        setCacheEntry(answers, guesses, remDepth, res.numWrong, res.numWrong, res.wordIndex, res.wordIndex);
    }

    inline void setLbCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        BestWordResult res,
        bool skipRecordSubset = false) {
        if (res.numWrong == 0) return;
        if (remDepth < GlobalArgs.minLbCache) return;
        setCacheEntry(answers, guesses, remDepth, res.numWrong, INF_INT, res.wordIndex, 0, skipRecordSubset);
    }

    void setCacheEntry(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        int lb, int ub, IndexType guessForLb, IndexType guessForUb, bool skipRecordSubset = false) {

        assert(std::is_sorted(answers.begin(), answers.end()));

        auto &cache = guessCache2[remDepth];
        std::pair<AnswersVec, GuessesVec> p = isEasyModeVar
            ? std::pair<AnswersVec, GuessesVec>(answers, GuessesVec())
            : std::pair<AnswersVec, GuessesVec>(answers, guesses);
        auto it = cache.find(p);
        if (it != cache.end()) {
            auto &curr = it->second;
            if (!skipRecordSubset) setSubsetCache(answers, guesses, remDepth, curr.lb, lb);
            if (lb > curr.lb) { curr.lb = lb; curr.guessForLb = guessForLb; }
            if (ub < curr.ub) { curr.ub = ub; curr.guessForUb = guessForUb; }
        } else {
            stats.addCacheWrite(remDepth);
            static const std::size_t entryOverhead = sizeof(std::_Rb_tree_node_base);
            static const std::size_t valueSize = sizeof(void*) + sizeof(LookupCacheEntry);

            std::size_t keySize = sizeof(void*) + answers.capacity() * sizeof(IndexType);
            cacheSize[remDepth] += entryOverhead + valueSize + keySize;
            // static std::ofstream cacheDebug("cacheDebug");
            // cacheDebug << answers.size() << '\n';
            // cacheDebug.flush();
            cache[p] = LookupCacheEntry(guessForLb, guessForUb, lb, ub);
            if (!skipRecordSubset) setSubsetCache(answers, guesses, remDepth, 0, lb);
        }
    }

    void setSubsetCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth,
        int oldLb, int lb)
    {
        if (!isRemDepthValidForSubsetCache(remDepth)) return;
    
        stats.tick(37);
        subsetCache.setSubsetCache(answers, guesses, remDepth, oldLb, lb);
        stats.tock(37);

        if (useBiTrieSubset(remDepth)) {
            stats.tick(39);
            biTrieSubsetCaches[remDepth].setSubsetCache(answers, guesses, oldLb, lb);
            stats.tock(39);

            // if (lb > 0) DEBUG("set value: " << lb << " " << getIterableString(answers));
        }
    }

    bool useBiTrieSubset(RemDepthType remDepth) {
        return remDepth == 3;
    }

    LookupCacheEntry getCache(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth) {
        stats.tick(46);
        auto &cache = guessCache2[remDepth];

        assert(std::is_sorted(answers.begin(), answers.end()));

        std::pair<AnswersVec, GuessesVec> p = isEasyModeVar
            ? std::pair<AnswersVec, GuessesVec>(answers, GuessesVec())
            : std::pair<AnswersVec, GuessesVec>(answers, guesses);
        auto it = cache.find(p);
        if (it == cache.end()) {
            stats.addCacheMiss(remDepth);
            stats.tock(46);
            return LookupCacheEntry::defaultLookupCacheEntry();
        }
        stats.addCacheHit(remDepth);
        stats.tock(46);
        return it->second;
    }

    bool isRemDepthValidForSubsetCache(const RemDepthType remDepth) const {
        //return !GlobalArgs.disableSubsetCache && remDepth == 4 && isEasyMode;
        return !GlobalArgs.disableSubsetCache && (remDepth == 3 || remDepth == 4) && isEasyMode;
    }

    int getLbFromAnswerSubsets(const AnswersVec &answers,
        const GuessesVec &guesses,
        const RemDepthType remDepth)
    {
        if (!isRemDepthValidForSubsetCache(remDepth)) return 0;
        int res = 0;

        if (useBiTrieSubset(remDepth)) {
            stats.tick(38);
            res = biTrieSubsetCaches[remDepth].getLbFromAnswerSubsets(answers, guesses);
            stats.tock(38);
            static bool compareWithSlowMethod = false;
            if (compareWithSlowMethod) {
                auto slowRes = subsetCache.getLbFromAnswerSubsetsFAST(answers, guesses, remDepth);
                if (slowRes != res) {
                    DEBUG("OH NO, exp: " << slowRes << ", actual: " << res);
                    DEBUG(getIterableString(answers));
                    exit(1);
                }
                
            }
        } else {
            stats.tick(36);
            res = subsetCache.getLbFromAnswerSubsetsFAST(answers, guesses, remDepth);
            stats.tock(36);
        }
    
        return res;
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
            long long approxBytes = 0;
            for (int i = 0; i <= maxTries; ++i) {
                approxBytes += cacheSize[i];
            }

            // DEBUG("approxBytes: " << stats.nodes << ", " << fromBytes(approxBytes));

            auto memLimit = 0.75 * GlobalArgs.memLimitPerThread * 1e9;
            if (approxBytes >= memLimit) {
                DEBUG("pruned ;)");
                for (int i = 0; i <= maxTries; ++i) {
                    cacheSize[i] = 0;
                    guessCache2[i].clear();
                }
            }
            nextCheck = stats.nodes + 1e6;

            // each cache entry = 
            // auto entrySize = (sizeof(void*) + keySize + valSize);
            // // DEBUG("entrySize: " << entrySize);
            // // DEBUG("sizeof(void*): " << sizeof(void*));
            // // DEBUG("keySize: " << keySize);
            // // DEBUG("valSize: " << valSize);
            // auto approxBytes = 1.1 * guessCache.size() * entrySize;
            // if (approxBytes >= GlobalArgs.memLimitPerThread * 1e9) {
            //     DEBUG("pruned ;)");
            //     guessCache.clear();
            // }
        }
    }

    int getGreenLetterMask(const AnswersVec &answers) {
        int greenLetterMask = 0;
        char greenLetters[5];
        uint8_t maxLetterCt[27] = {0}, greenLetterCt[27] = {0};
        for (auto i = 0; i < 5; ++i) greenLetters[i] = GlobalState.reverseIndexLookup[answers[0]][i];
        for (auto answerIndex: answers) {
            uint8_t letterCt[27] = {0};
            auto &answerWord = GlobalState.reverseIndexLookup[answerIndex];
            for (int i = 0; i < 5; ++i) {
                if (greenLetters[i] != answerWord[i]) greenLetters[i] = '.';
                letterCt[letterToInd(answerWord[i])]++;
            }
            for (int i = 0; i < 5; ++i) {
                maxLetterCt[letterToInd(answerWord[i])] = std::max(maxLetterCt[letterToInd(answerWord[i])], letterCt[letterToInd(answerWord[i])]);
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
                greenLetterMask |= (1 << (greenLetters[i] - 'a'));
            }
        }
        return greenLetterMask;
    }
};
