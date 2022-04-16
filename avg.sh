set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
./bin/solve -pr --lowest-average -w crane ext/wordle-guesses.txt ext/wordle-answers.txt