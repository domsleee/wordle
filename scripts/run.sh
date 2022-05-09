set -e;
make clean;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#./bin/solve -pr --max-tries 4 --max-incorrect 33 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --max-tries 4 --max-incorrect 146 ext/wordle-guesses.txt ext/wordle-answers.txt

#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
#./bin/solve -pr --force-sequential --max-tries 1 --max-incorrect 150 -w slate ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -r --max-tries 3 --max-incorrect 150 -w slate --force-sequential ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -r --max-tries 4 --max-incorrect 33 -w caste --guesses-to-check databases/awake ext/wordle-guesses.txt ext/wordle-answers.txt
# 33 --> 31s
#./bin/solve -p -w salet ext/wordle-guesses.txt ext/wordle-answers.txt
./bin/solve -p -N2 -I0 -g6 ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -lp -I50 -w rance ext/wordle-combined.txt ext/wordle-combined.txt

#./bin/solve -p -I3147 -t4-w rance ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -p -I18 -t4 -w rance ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p -I0 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -pr --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --hard-mode -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt


