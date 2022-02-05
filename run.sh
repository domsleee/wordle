set -e;
make clean;
make -j;
env CPUPROFILE=out2.prof ./bin/solve ext/wordle-guesses.txt ext/wordle-answers.txt