#include "../include/casino.h"
#include "math.h"

double CAS_WinRate(struct CAS_Node* n)
{
	return (n->wins + 0.5*n->draws) / n->playouts;
}

double CAS_UCBExploration(struct CAS_Node* n, double explorationFactor)
{
	return explorationFactor * sqrt(log(n->parent->playouts) / n->playouts);
}

struct CAS_Node* CAS_SelectByScore(struct CAS_Node* n,
                                   double (*SelectScore)(struct CAS_Node*))
{
    struct CAS_Node* selected = NULL, *current;
    double score, bestScore;
    size_t i;

    bestScore = 0;
    for (i = 0; i < n->children->numNodes; i++)
    {
        current = &n->children->nodes[i];

        if (current->playouts == 0)
            return current;

        score = SelectScore(current);
        if (score >= bestScore)
        {
            selected = current;
            bestScore = score;
        }
    }

    return selected;
}

double DefaultNodeScore(struct CAS_Node* n)
{
    const double ExplorationConstant = sqrt(2);
    return CAS_WinRate(n) + CAS_UCBExploration(n, ExplorationConstant);
}

struct CAS_Node* CAS_DefaultSelectionPolicy(void* cas,
                                            struct CAS_Node* n)
{
    (void)cas;
    return CAS_SelectByScore(n, &DefaultNodeScore);
}
