set -ex;

FLAGS_GEN="-fprofile-generate=./prof/out_single.pgo -lgcov"
FLAGS_USE="-fprofile-use=./prof/out_single.pgo -lgcov"

make clean
ENV_CXXFLAGS="$FLAGS_GEN" ENV_LIBFLAGS="$FLAGS_GEN" make -j
./bin/solve -r --lowest-average --force-sequential -w crane --guesses-to-check databases/wordsAvgPgo_easy ext/wordle-guesses.txt ext/wordle-answers.txt

make clean
ENV_CXXFLAGS="$FLAGS_USE" ENV_LIBFLAGS="$FLAGS_USE" make -j

#./bin/solve --max-tries 3 --max-incorrect 974 -p ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --max-tries 4 --max-incorrect 33 ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --max-tries 4 --max-incorrect 33 ext/wordle-guesses.txt ext/wordle-answers.txt

./bin/solve -p -w crane --lowest-average ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -pr --hard-mode --lowest-average -w scowl ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -p --lowest-average -w crane ext/wordle-guesses.txt ext/wordle-answers.txt
#./bin/solve -pr --lowest-average -w crane ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -p --lowest-average -w adieu ext/wordle-guesses.txt ext/wordle-answers.txt

#./bin/solve -pr --hard-mode --max-tries 5 ext/wordle-guesses.txt ext/wordle-answers.txt