set -e;
make -j;
#./bin/solve --max-tries 2 --max-incorrect 500 -p "ext/wordle-guesses.txt" "ext/wordle-answers.txt"


# use -r for profiling
# 989
#./bin/solve -pr --max-tries 4 --max-incorrect 33 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --max-tries 4 --max-incorrect 146 ext/wordle-guesses.txt ext/wordle-answers.txt

./bin/solve -Seartolsincuyhpdmgbkf -p -L 1.8 -W 12 ext/wordle-combined.txt ext/wordle-combined.txt
# ./bin/solve -p -Seartolsincuyhpdmgbkf -L 1 -N50 --min-lb-cache 2 ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -Seartolsincuyhpdmgbkf -w lants ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -Seartolsincuyhp -L 1 -N20 -n1150 --min-lb-cache 2 ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve --min-lb-cache 3 -Seartolsincuy -L25 -I50 -g4 -N50 ext/wordle-guesses.txt ext/wordle-answers.txt

#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
#./bin/solve -L20 -w tenor -I17 -g4 -Seartolsincuyhpdmgbkf ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -Seartolsincuyhpdmgbkf -N5 ext/wordle-combined.txt ext/wordle-combined.txt
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
#./bin/solve -Seartolsincuyhp -L 1 -N20 -n1150 --min-lb-cache 2 ext/wordle-combined.txt ext/wordle-combined.txt
#/bin/solve --min-lb-cache 2 -Seartolsincuyhpdmgbkf -N20 -g4 -I50 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -Seartolsincuyhpdmgbkf -I50 -g4 -N1 ext/wordle-guesses.txt ext/wordle-answers.txt

#LSAN_OPTIONS=fast_unwind_on_malloc=1 ./bin/solve -Seartol -p -I20 -g4 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --force-sequential --max-tries 1 --max-incorrect 150 -w slate ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -r --max-tries 3 --max-incorrect 150 -w slate --force-sequential ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -r --max-tries 4 --max-incorrect 33 -w caste --guesses-to-check databases/awake ext/wordle-guesses.txt ext/wordle-answers.txt
# 33 --> 31s
#./bin/solve -p -w salet ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p -N2 -I1 -n5 ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -lp -I50 -w rance ext/wordle-combined.txt ext/wordle-combined.txt

#./bin/solve -p -I3147 -t4-w rance ext/wordle-combined.txt ext/wordle-combined.txt
#./bin/solve -p -I18 -t4 -w rance ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p -I0 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -pr --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --hard-mode -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt


