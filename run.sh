set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#./bin/solve -pr --max-tries 4 --max-incorrect 33 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --max-tries 4 --max-incorrect 146 ext/wordle-guesses.txt ext/wordle-answers.txt

#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
#./bin/solve -p --max-tries 4 --max-incorrect 33 -w caste --guesses-to-check databases/awake ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --hard-mode -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
./bin/solve -pr --max-tries 6 --max-incorrect 0 -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt

