set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=out11.prof \
./bin/solve --max-tries 3 --max-incorrect 500 -r ext/wordle-guesses.txt ext/wordle-answers.txt