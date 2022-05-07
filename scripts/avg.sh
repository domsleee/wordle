set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
./bin/solve -lp -N2 -n5 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -lp -w salet ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -pr --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --lowest-average -w crane ext/wordle-guesses.txt ext/wordle-answers.txt
