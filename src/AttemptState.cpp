#include "../include/AttemptState.h"

AttemptState::CacheType<WordSetGuesses> AttemptState::wsGuessCache = {};
AttemptState::CacheType<WordSetAnswers> AttemptState::wsAnswerCache = {};
long long AttemptState::cacheHit, AttemptState::cacheMiss, AttemptState::cacheSize;
