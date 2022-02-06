#include "../include/AttemptState.h"

AttemptState::CacheType AttemptState::attemptStateCache = {};
long long AttemptState::cacheHit, AttemptState::cacheMiss, AttemptState::cacheSize;
bool AttemptState::suppressErrors = false;
AttemptState::ReverseIndexType AttemptState::reverseGuessToIndex = {};