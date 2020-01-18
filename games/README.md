# Example implementations
This folder contains some example game implementations which exercise the Casino library. Hopefully these will be a good starting point for new users. Each game is fully playable from the command line, though the AI will always move first.

## Tic-tac-toe
This game is sufficiently simple that Casino can play it optimally without any changes from the default configuration and with very modest resources.

## Breakthrough
Breakthrough is an attractive application for MCTS because it is difficult to evaluate the board state statically. Casino plays very badly with the default configuration - I believe this is because the game can last for many turns and this reduces the correlation between a move's utility and its playout results.

In order to improve this program's performance I have provided custom selection and playout policies which embed some domain specific knowledge.

With these changes this program plays at an intermediate level though it still falls for straightforward tactics sometimes.

### Biased selection policy
I have implemented a method called _progressive bias_ in the selection policy which initially weights capturing moves more highly. As a move gains more playouts this bias reduces, and in the limit the standard UCB formula is used.

### Biased playout policy
The playout policy has three stages:
1. If a winning move is available then play it.
2. If capturing moves are available then play a random one with high probability.
3. If neither of the above yields a move then use the default uniform policy.

## Amazons
[Amazons](https://en.wikipedia.org/wiki/Game_of_the_Amazons) is a territorial game that is somewhat similar to Go. It has a very high branching factor (initially 2176 moves!) and it is difficult to evaluate the board state during the opening and middlegame.

In order to deal with the high branching factor I have broken down Amazons actions into 3 stages which significantly reduces the breadth of the game tree:

* Piece selection
* Piece movement
* Firing an arrow

These are essentially selected independently during MCTS and then the stages are glued together at the end.

With this improvement the program plays at the level of an intermediate player, but as in Breakthrough it still has tactical weaknesses.
