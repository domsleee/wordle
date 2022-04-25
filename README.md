# Wordle

Uses ideas from these notes:
* http://sonorouschocolate.com/notes/index.php?title=The_best_strategies_for_Wordle

Produces model files `/models` in this structure:
* https://jonathanolson.net/experiments/optimal-wordle-solutions

## Usage

See `./bin/solve -h`.

```
A solver for the wordle game.
Usage:
  WordleSolver [OPTION...] positional parameters

  -H, --hard-mode              Solve the hard mode of wordle
  -l, --lowest-average         Find a strategy for the lowest average score 
                               (default is least wrong)
  -p, --parallel               Use parallel processing
  -r, --reduce-guesses         Use the answer dictionary as the guess 
                               dictionary
  -s, --force-sequential       No parallel when generating cache (for 
                               profiling)
  -w, --first-guess arg        First guess (default: "")
  -v, --verify arg             Solution model to verify (default: "")
      --guesses-to-skip arg    Filename of words to skip (default: "")
      --guesses-to-check arg   Filename of guesses to check (default: "")
  -m, --max-tries arg          Max Tries for least wrong strategy (default: 
                               6)
  -i, --max-incorrect arg      Max incorrect for lowest average strategy 
                               (default: 0)
      --max-total-guesses arg  Max total guesses (default: 500000)
      --num-to-restrict arg    Reduce the number of first guesses (default: 
                               50000)
  -h, --help                   Print usage
```

## Use cases

Find _any_ strategy that solves all words, with first guess caste:
```
./bin/solve -p --max-incorrect 0 -w salet ext/wordle-guesses.txt ext/wordle-answers.txt
```

Find the strategy with the smallest expected number of guesses, with first word caste (note: slow):
```
./bin/solve -p --lowest-average -w salet --max-total-guesses 7950 ext/wordle-guesses.txt ext/wordle-answers.txt
```
Note: `--max-total-guesses` here is based on the correct solution on 7920. It reduces the search space by filtering out trees that have a total greater than the value of the arg.

## Implementation Details

The implementation uses `std::bitset` to represent sets of word indexes, eg `WordSetGuesses` and `WordSetAnswers`.
The size of these bitsets are tied to `MAX_NUM_GUESSES` and `NUM_WORDS` at compile time, so these constants need to be adjusted per bitset.
