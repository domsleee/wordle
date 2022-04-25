rm -r obj/* bin/*
g++-11  -fmax-errors=1 -std=c++20 -Wall -I third_party -O3 -DNDEBUG -g -ffast-math    -c -o obj/JsonConverter.o src/JsonConverter.cpp -ltbb 
g++-11  -fmax-errors=1 -std=c++20 -Wall -I third_party -O3 -DNDEBUG -g -ffast-math    -c -o obj/Runner.o src/Runner.cpp -ltbb 
g++-11  -fmax-errors=1 -std=c++20 -Wall -I third_party -O3 -DNDEBUG -g -ffast-math    -c -o obj/PatternGetter.o src/PatternGetter.cpp -ltbb 
g++-11  -fmax-errors=1 -std=c++20 -Wall -I third_party -O3 -DNDEBUG -g -ffast-math    -c -o obj/main.o src/main.cpp -ltbb 
g++-11 -fmax-errors=1 -std=c++20 -Wall -I third_party -O3 -DNDEBUG -g -ffast-math    -ltbb  -o bin/solve obj/JsonConverter.o obj/Runner.o obj/PatternGetter.o obj/main.o -ltbb 
error: does not have next pattern '+++_+'
curren guess: booby
+++++
answer: boozy
$.next["_____"].next["__+__"].next["+___+"]
amount: 1
error: does not have next pattern '_++++'
curren guess: fluff
+++++
answer: bluff
$.next["_____"].next["___?_"].next["_?___"]
amount: 1
error: does not have next pattern '?+_++'
curren guess: lowly
+++++
answer: folly
$.next["_____"].next["__?+_"].next["_+___"]
amount: 1
error: does not have next pattern '_++++'
curren guess: bowel
+++++
answer: vowel
$.next["____?"].next["_?__+"].next["?__+_"]
amount: 1
error: does not have next pattern '_+__+'
curren guess: foggy
+++++
answer: poppy
$.next["_____"].next["__?__"].next["____+"]
amount: 1
error: does not have next pattern '_?++?'
curren guess: gruel
+++++
answer: bluer
$.next["?___?"].next["_____"].next["??___"]
amount: 1
error: does not have next pattern '?+_++'
curren guess: lowly
+++++
answer: jolly
$.next["_____"].next["__?+_"].next["_+___"]
amount: 1
error: does not have next pattern '_++++'
curren guess: poker
+++++
answer: joker
$.next["?___?"].next["_+___"].next["__?__"]
amount: 1
error: does not have next pattern '_??_+'
curren guess: derby
+++++
answer: every
$.next["?___?"].next["_____"].next["____+"]
amount: 1
error: does not have next pattern '?++_+'
curren guess: husky
+++++
answer: bushy
$.next["_____"].next["?____"].next["___??"]
amount: 1
error: does not have next pattern '_+_++'
curren guess: poker
+++++
answer: boxer
$.next["?___?"].next["_+___"].next["__?__"]
amount: 1
error: does not have next pattern '_++++'
curren guess: kitty
+++++
answer: bitty
$.next["_____"].next["____?"].next["_++++"]
amount: 1
error: does not have next pattern '_++_+'
curren guess: derby
+++++
answer: jerky
$.next["?___?"].next["_____"].next["____+"]
amount: 1
error: does not have next pattern '_++++'
curren guess: kitty
+++++
answer: witty
$.next["_____"].next["____?"].next["_++++"]
amount: 1
error: does not have next pattern '++_++'
curren guess: waver
+++++
answer: wafer
$.next["?+__?"].next["?____"].next["_____"]
amount: 1
error: does not have next pattern '_++?+'
curren guess: derby
+++++
answer: berry
$.next["?___?"].next["_____"].next["____+"]
amount: 1
error: does not have next pattern '_++++'
curren guess: breed
+++++
answer: greed
$.next["?___?"].next["_____"].next["_____"]
amount: 1
error: does not have next pattern '+?_++'
curren guess: slyly
+++++
answer: sully
$.next["_____"].next["+__+_"].next["___?_"]
amount: 1
error: does not have next pattern '_++++'
curren guess: hilly
+++++
answer: willy
$.next["_____"].next["___+_"].next["_+___"]
amount: 1
above4: 0
=============
maxTries    : 6
maxIncorrect: 0
valid?      : No (19 <= 0)
easy mode   : Yes
isLowAverage: No
correct     : 2296/2315 (99.18%)
guess words : 12972
firstWord   : caste
average     : 3.69294
solver cache: 0/0 (inf%)
