set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
#./bin/solve -pr --max-tries 4 --max-incorrect 33 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --max-tries 4 --max-incorrect 146 ext/wordle-guesses.txt ext/wordle-answers.txt

./bin/solve -pr --hard-mode --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --hard-mode -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
