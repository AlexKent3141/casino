
# Casino library design
This document contains the high-level design details for the Casino library.

## Requirements
1. It must be possible to apply the library to a wide variety of problem domains.
2. It must be possible to customise which MCTS methods/extensions are applied to each domain, including:
* The criteria for move selection during the selection and playout stages of the algorithm.
* Expansion criteria.
* Whether RAVE is used.
* ...
3. The library must be highly performant.
* Multi-threading?
5. The library must be cross-platform

## Design choices
In order to achieve maximum performance and make this library as reusable as possible I want to implement Casino in C. Once this is done it will be possible to wrap Casino in a higher-level language e.g. Python.

The C programming language lacks the ability to operate on generic types, so in order to make the MCTS options customisable function pointers will probably need to be used extensively.

For example, I envisage that the problem domain that is being targeted could be specified in the following way (the domain data will need to be passed around in a `void*`):
```c	
void* make_state();
action_list* get_actions(void*);
void do_action(void*, action);
```
[Note: I think it is reasonable to assume that actions can be encoded in a 64-bit integer, which means that the `action_list` can be implemented in the Casino library.]

Pointers to these functions will need to be passed to the library.
