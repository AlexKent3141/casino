#include "mcts.h"

/* Keep selecting child nodes until a leaf node (or a terminal node) is reached. */
struct CAS_Node* Select(
    void* cas,
    struct CAS_SearchConfig* config,
    struct CAS_Domain* domain,
    struct CAS_Node* n,
    CAS_DomainState position)
{
    struct CAS_Node* selected = n;

    while (selected->expanded
        && selected->children != NULL
        && selected->children->numNodes > 0)
    {
        selected = config->SelectionPolicy(cas, position, selected);
        domain->DoAction(position, selected->action);
    }

    return selected;
}

/* Add nodes for each of the actions available from n and pick the first. */
struct CAS_Node* Expand(
    struct CAS_State* cas,
    struct CAS_Domain* domain,
    struct CAS_Node* n,
    CAS_DomainState position,
    struct CAS_ActionList* actionList)
{
    struct CAS_Node* expanded = n;
    enum CAS_Player nextPlayer;
    int nextActionStage;
    size_t i;

    actionList->numActions = 0;
    domain->GetStateActions(position, actionList);
    if (actionList->numActions > 0)
    {
        /* Work out who the next player is taking into account action stages. */
        nextPlayer = n->player;
        nextActionStage = (n->stage + 1) % domain->actionStages;
        if (nextActionStage == 0)
            nextPlayer = nextPlayer == CAS_P1 ? CAS_P2 : CAS_P1;

        n->children = GetNodeList(cas->mem, actionList->numActions);
        if (n->children == NULL)
        {
            return NULL;
        }

        for (i = 0; i < actionList->numActions; i++)
        {
            AddNode(
                n->children,
                n,
                nextPlayer,
                nextActionStage,
                actionList->actions[i]);
        }

        expanded = &n->children->nodes[0];
    }

    n->expanded = true;

    return expanded;
}

bool CAS_DefaultStopPlayoutCriterion(
    CAS_Action previousAction,
    int playoutDepth)
{
    (void)playoutDepth;
    return previousAction == CAS_BAD_ACTION;
}

/* Play out a random game from the point and score the terminal state. */
enum CAS_Player Simulate(
    struct PRNGState* st,
    struct CAS_SearchConfig* config,
    struct CAS_Domain* domain,
    CAS_DomainState position,
    struct CAS_ActionList* actionList)
{
    CAS_Action action;
    int depth = 0;

    actionList->numActions = 0;
    action = config->PlayoutPolicy(st, domain, position, actionList);
    while (!config->StopPlayout(action, depth))
    {
        domain->DoAction(position, action);

        /* Select which move to make next using the playout policy. */
        actionList->numActions = 0;
        action = config->PlayoutPolicy(st, domain, position, actionList);

        ++depth;
    }

    return domain->GetScore(position);
}

void Backprop(
    struct CAS_Node* n,
    enum CAS_Player winner)
{
    do
    {
        /* A node's score is a reflection of how good the move to reach that node
           was i.e. a move for the other player. This is why we check against the
           parent node's player below. */
        n->playouts++;
        if (n->parent != NULL)
        {
            if (winner == CAS_NONE)
                n->draws++;
            else if (n->parent->player == winner)
                n->wins++;
        }

        n = n->parent;

    } while (n != NULL);
}

void* SearchWorker(void* threadData)
{
    struct WorkerData* data = (struct WorkerData*)threadData;
    struct CAS_Node* n, *selected;
    enum CAS_Player winner;
    struct CAS_State* cas = data->cas;
    struct CAS_Domain* domain = cas->domain;

    /* Perform the MCTS procedure while resources remain. */
    /* All access to the tree is synchronised by the shared mutex. */
    do
    {
        domain->CopyState(
            data->initialPosition,
            data->workerPosition);

        pthread_mutex_lock(data->treeLock);

        selected = Select(
            cas,
            data->config,
            cas->domain,
            cas->root,
            data->workerPosition);

        n = Expand(
            cas,
            cas->domain,
            selected,
            data->workerPosition,
            data->actionList);

        pthread_mutex_unlock(data->treeLock);

        if (n == NULL)
        {
            /* Out of memory, just keep doing playouts on the current tree. */
            n = selected;
        }

        winner = Simulate(
            data->prngState,
            data->config,
            cas->domain,
            data->workerPosition,
            data->actionList);

        pthread_mutex_lock(data->treeLock);

        Backprop(n, winner);

        pthread_mutex_unlock(data->treeLock);

        pthread_testcancel();

    } while (true);

    return NULL;
}
