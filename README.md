# Casino
Casino is a library which implements the MCTS (Monte Carlo Tree Search) algorithm. The main objective of this library is to allow MCTS to be applied to a wide range of applications and to support as many extensions of MCTS as possible, while maintaining a high level of performance.

## Motivation
The MCTS algorithm has been shown to be very effective in some areas where other search algorithms (e.g. Minimax) have not performed well. It has been the state-of-the-art for Go AI implementations for some time, most recently in conjunction with Neural Networks (as is the case in Deepmind's AlphaGo).

My MCTS implementation in OnePunchGo is very much Go-specific. This library is an attempt to make something more reusable.

## Features
* Fast single-threaded MCTS implementation with UCB-based exploration/exploitation.
* Ability to apply MCTS to a wide range of problems.

## TODO
Add more MCTS extensions:
* Customisable move selection criteria for the selection and expansion phases of the algorithm.
* RAVE option.
* Different number of players (single player or even > 2 players).
* Support for imperfect information games.

Optimisations:
* Multi-threading.

## Design
The problem domain is specified by defining a `CAS_Domain` struct. This struct contains pointers to functions that implement the domain rules such as move generation and scoring.

One unusual feature of Casino is that it does not perform any dynamic memory allocation. The application must allocate a single buffer of memory and pass this to Casino, which will use the memory as it sees fit. This makes Casino much easier to maintain - it is much more difficult to introduce memory leaks!

The search routine will keep running until it runs out of resources i.e. available memory or time.

## Building
I have only tested Casino on Linux. I think it should compile under Cygwin on Windows but this is currently untested. The code itself is written in C89 and only depends on the standard library so it should be completely portable.

### Casino library
The Casino library can be compiled by running `make` in the root directory. This will produce the libcasino.so shared library which can be linked against by applications.

All of the exported functions are defined in the casino.h header in the include folder.

### Unit tests
The tests can be compiled by running `make test` in the root directory.

### Tic-tac-toe example
The tic-tac-toe example can be compiled by running `make tictactoe` in the root directory. Running the tictactoe binary will allow you to play against the AI in the command line.
