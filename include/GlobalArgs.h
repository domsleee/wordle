#pragma once
struct _GlobalArgs {
    static inline bool forceSequential = false;
};

static inline auto GlobalArgs = _GlobalArgs();
