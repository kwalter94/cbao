#include "eval.h"
#include "error.h"
#include "tree.h"

#include <stdlib.h>


static int get_best_score(Player, BaoTree*, const BaoRules*, int);

// TODO: Use minimax/negamax with alpha - beta  pruning
int best_branch(BaoTree *node, const BaoRules *rules, int depth)
{
	int i, best_path, max_score, score;

	if(grow_tree(node, rules) == -1)
		return -1;
	best_path = -1;
	max_score = -1000;
	for(i = 0; i < node->nchildren; i++) {
		score = get_best_score(node->state.player, node->children[i],
				rules, depth);
		if(score == max_score) {
			best_path = (rand() * 10) % 2 ? best_path : i;
		} else if(score > max_score) {
			max_score = score;
			best_path = i;
		}
	}
	return best_path;
}

static int eval_branch(const BaoTree *node, Player player)
{
	Hole h;
	int score;

	if(node->nchildren == 0)
		/* Best or worst that can happen. If node->state.player != player then
		 * opponent is out of moves else it's player that's out of moves. */
		return 0;
	score = 0;
	for(h = 0; h < H_STORE; h++)
		score += node->state.board[node->state.player][h];
	if(player != node->state.player)
		score = -score;
	return score;
}

static int get_best_score(Player player, BaoTree *node, const BaoRules *rules,
		int depth)
{
	int i, score, best_score;

	if(grow_tree(node, rules) == -1)
		choke("get_best_score(): grow_tree_failed");
	if(depth == 0)
		return eval_branch(node, player);
	best_score = 0;
	for(i = 0; i < node->nchildren; i++) {
		score = get_best_score(player, node->children[i], rules, depth - 1);
		if(score > best_score)
			best_score = score;
		free_tree(node->children[i]);
		node->children[i] = NULL;
	}
	node->nchildren = 0;
	return best_score;
}



