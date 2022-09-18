set -e;
make -j;

#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libprofiler.so env CPUPROFILE=./prof/out.prof \
./bin/solve --guesses ext/wordle-combined.txt --answers ext/wordle-combined.txt -Seart

