# Casino
Casino is a library which implements the MCTS (Monte Carlo Tree Search) algorithm. The main objective of this library is to allow MCTS to be applied to a wide range of applications and to support as many extensions of MCTS as possible, while maintaining a high level of performance.

## Motivation
The MCTS algorithm has been shown to be very effective in some areas where other search algorithms (e.g. Minimax) have not performed well. It has been the state-of-the-art for Go AI implementations for some time, most recently in conjunction with Neural Networks (as is the case in Deepmind's AlphaGo).

My MCTS implementation in OnePunchGo is very much Go-specific. This library is an attempt to make something more reusable.

## Features
* Fast multi-threaded MCTS implementation with UCB-based exploration/exploitation.
* Ability to apply MCTS to a wide range of problems.
* Customisable selection and playout policies.
* Move groups (see Amazons example).

## TODO
Add more MCTS extensions:
* RAVE option.
* Different number of players (single player or even > 2 players).
* Support for imperfect information games.

## Design
The problem domain is specified by defining a `CAS_Domain` struct. This struct contains pointers to functions that implement the domain rules such as move generation and scoring.

Casino does not perform any dynamic memory allocation. The application must allocate a single buffer of memory and pass this to Casino, which will use the memory as it sees fit. This makes Casino much easier to maintain - it is much more difficult to introduce memory leaks!

The search routine will keep running until it runs out of time. If it runs out of memory it will keep simulating its existing tree.

## Building
Casino is written in C89 and only depends on the standard library so it should be completely portable. It is built using cmake and has been tested on Windows and Linux.

For example, from the root directory you can run the commands (on Linux):

`mkdir build`

`cd build`

`cmake ..`

`cmake --build . --config Release`

This will build the Casino library and all of the games.
