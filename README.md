# Wordle

Uses ideas from these notes:
* http://sonorouschocolate.com/notes/index.php?title=The_best_strategies_for_Wordle

Produces model files `/models` in this structure:
* https://jonathanolson.net/experiments/optimal-wordle-solutions

### Use cases

Find _any_ strategy that solves all words, with first guess caste:
```
./bin/solve -p --max-incorrect 0 -w salet ext/wordle-guesses.txt ext/wordle-answers.txt
```

Find the strategy with the smallest expected number of guesses, with first word caste (note: slow):
```
./bin/solve -p --lowest-average -w salet --max-total-guesses 7950 ext/wordle-guesses.txt ext/wordle-answers.txt
```
Note: `--max-total-guesses` here is based on the correct solution on 7920. It reduces the search space by filtering out trees that have a total greater than the value of the arg.



