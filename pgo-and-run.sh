set -ex;

FLAGS_GEN="-fprofile-generate=./prof/out_single.pgo -lgcov"
FLAGS_USE="-fprofile-use=./prof/out_single.pgo -lgcov"

make clean
ENV_CXXFLAGS="$FLAGS_GEN" ENV_LIBFLAGS="$FLAGS_GEN" make -j
./bin/solve --max-tries 3 --max-incorrect 150 -w slate -r --force-sequential ext/wordle-guesses.txt ext/wordle-answers.txt

make clean
ENV_CXXFLAGS="$FLAGS_USE" ENV_LIBFLAGS="$FLAGS_USE" make -j

#./bin/solve --max-tries 3 --max-incorrect 974 -p ext/wordle-guesses.txt ext/wordle-answers.txt
./bin/solve --max-tries 4 --max-incorrect 33 -p -r ./databases/easy2315_4 ext/wordle-guesses.txt ext/wordle-answers.txt
