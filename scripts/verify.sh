
make clean;
make -j;
./bin/solve -v ./models/pretty.json ./ext/wordle-combined.txt ./ext/wordle-combined.txt
#./bin/solve -v ./models/rance.converted.json ./ext/wordle-guesses.txt ./ext/wordle-answers.txt