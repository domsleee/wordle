set -e;
make clean;
make -j;
LD_PRELOAD=/opt/homebrew/lib/libprofiler.dylib env CPUPROFILE=out11.prof ./bin/solve ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve ext/wordle-guesses.txt ext/wordle-answers.txt