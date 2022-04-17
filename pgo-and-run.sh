set -ex;

FLAGS_GEN="-fprofile-generate=./prof/out_single.pgo -lgcov"
FLAGS_USE="-fprofile-use=./prof/out_single.pgo -lgcov"

make clean
ENV_CXXFLAGS="$FLAGS_GEN" ENV_LIBFLAGS="$FLAGS_GEN" make -j
./bin/solve -r --max-tries 3 --max-incorrect 150 -w slate --force-sequential ext/wordle-guesses.txt ext/wordle-answers.txt

make clean
ENV_CXXFLAGS="$FLAGS_USE" ENV_LIBFLAGS="$FLAGS_USE" make -j

#./bin/solve --max-tries 3 --max-incorrect 974 -p ext/wordle-guesses.txt ext/wordle-answers.txt
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
./bin/solve -pr --max-tries 4 --max-incorrect 33 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -r --max-tries 3 --max-incorrect 150 -w slate --force-sequential ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --max-tries 4 --max-incorrect 33 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --max-tries 4 --max-incorrect 22 -w caste ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --hard-mode --max-tries 3 --max-incorrect 1122 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --max-tries 6 --max-incorrect 0 ext/wordle-guesses.txt ext/wordle-answers.txt