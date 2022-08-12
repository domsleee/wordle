# Wordle

Uses ideas from these notes:
* http://sonorouschocolate.com/notes/index.php?title=The_best_strategies_for_Wordle

Produces model files `/models` in this structure:
* https://jonathanolson.net/experiments/optimal-wordle-solutions

## Usage

See `./bin/solve -h` for usage.

## Use cases

Find _any_ strategy that solves all words (with a maximum of 0 incorrect), with first guess salet:
```
./bin/solve -I0 -w salet --guesses ext/wordle-guesses.txt --answers ext/wordle-answers.txt
```

Find _any_ strategy that solves all words with 4 guesses and a maximum of 20 incorrect, using a special string lookup to remove inferior guesses at each node:
```
./bin/solve -p -Seartolsinc -I20 -g4 --guesses ext/wordle-guesses.txt --answers ext/wordle-answers.txt
```

Finding the lowest average is not supported, it can be done on [my fork of alex1770's repo](https://github.com/domsleee/wordle-1).
