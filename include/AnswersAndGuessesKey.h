#include "WordSetUtil.h"
#include "Util.h"

struct AnswersAndGuessesKey {
    const MinLetterType minLetterLimit;
    const WordSetAnswers wsAnswers;
    const WordSetGuesses wsGuesses;
    const int tries, triesRemaining;
    AnswersAndGuessesKey(const MinLetterType &minLetterLimit, const WordSetAnswers &wsAnswers, const WordSetGuesses &wsGuesses, int tries, int triesRemaining)
        : minLetterLimit(minLetterLimit),
          wsAnswers(wsAnswers),
          wsGuesses(wsGuesses),
          tries(tries),
          triesRemaining(triesRemaining) {}

    friend bool operator==(const AnswersAndGuessesKey &a, const AnswersAndGuessesKey &b) {
        return a.minLetterLimit == b.minLetterLimit 
          && a.wsAnswers == b.wsAnswers
          && a.wsGuesses == b.wsGuesses
          && a.tries == b.tries
          && a.triesRemaining == b.triesRemaining;
    }
};


template <>
struct std::hash<AnswersAndGuessesKey> {
    std::size_t operator()(const AnswersAndGuessesKey& k) const {
        size_t res = 17;
        long long ct = 0, mult = 1;
        for (int i = 0; i < 26; ++i) {
            ct += k.minLetterLimit[i] * mult;
            mult *= 5;
        }
        res = res * 31 + hash<long long>()( ct );
        res = res * 31 + hash<WordSetAnswers>()( k.wsAnswers );
        res = res * 31 + hash<WordSetGuesses>()( k.wsGuesses );
        res = res * 31 + hash<int>()( k.tries );
        res = res * 31 + hash<int>()( k.triesRemaining );
        return res;
    }
};

