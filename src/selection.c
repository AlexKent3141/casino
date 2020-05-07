#include "../include/casino.h"
#include "math.h"

double CAS_WinRate(struct CAS_Node* n)
{
	return (n->wins + 0.5*n->draws) / n->playouts;
}

double CAS_UCBExploration(
    struct CAS_Node* n,
    double explorationFactor)
{
	return explorationFactor * sqrt(log(n->parent->playouts) / n->playouts);
}

struct CAS_Node* CAS_SelectByScore(
    struct CAS_Node* parent,
    CAS_DomainState position,
    double (*SelectScore)(
        CAS_DomainState,
        struct CAS_Node*))
{
    struct CAS_Node* selected = NULL, *current;
    double score, bestScore = 0;
    size_t i;

    for (i = 0; i < parent->children->numNodes; i++)
    {
        current = &parent->children->nodes[i];

        if (current->playouts == 0)
            return current;

        score = SelectScore(position, current);
        if (score >= bestScore)
        {
            selected = current;
            bestScore = score;
        }
    }

    return selected;
}

double DefaultNodeScore(
    CAS_DomainState position,
    struct CAS_Node* n)
{
    const double ExplorationConstant = sqrt(2);
    (void)position;
    return CAS_WinRate(n) + CAS_UCBExploration(n, ExplorationConstant);
}

struct CAS_Node* CAS_DefaultSelectionPolicy(
    void* cas,
    CAS_DomainState position,
    struct CAS_Node* parent)
{
    (void)cas;
    return CAS_SelectByScore(parent, position, &DefaultNodeScore);
}
