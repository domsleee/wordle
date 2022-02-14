set -e;
make clean;
make -j;
# LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=out11.prof ./bin/solve ext/wordle-guesses.txt ext/wordle-answers.txt
./bin/solve ext/wordle-guesses.txt ext/wordle-answers.txt