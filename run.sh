set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=out11.prof \
./bin/solve --max-tries 4 --max-incorrect 150 -r -p ext/wordle-guesses.txt ext/wordle-answers.txt