/******************************************************************************
 *	BaoTree.c: An implementation of the bao game logic
 *
 *		Module mainly provides routines for generating and traversing
 *		a bao game tree. Additional functions are included for examining
 *		the step by step execution of a move.
 *
 *		NOTE: Through out this module, takata refers to all non-capture
 * 		moves, capture (not mtaji) refers to all capture moves and the word
 *		mtaji is used to refer to the second stage in the advanced bao
 *		player mode which is preceeded by the namua stage.
 *
 *		For information on the rules of bao and more see:
 *			1) http://www.fdg.unimaas.nl/educ/donkers/games/Bao
 * 			2) http://www.gamecabinet.com/rules/BaoIntro.html
 *			3) http://en.wikipedia.org/wiki/Bao_(mancala_game)
 *			4) DuckDuckgo is a brother ask him...
 *
 *	Author: Walter Nyirenda (walterkaunda@gmail.com)
 *
 *	License: GPLv3++
 *
 *		A copy of the license *must* be available where you got this (whole)
 *		from (a folder or some archive or wherever), if not, am pretty much
 *		certain you will always find it at:
 *				http://www.gnu.org/licenses/gpl.txt
 *
 * NOTE: The documentation may be out of sync with the object(s) it is trying
 *		 trying to describe. I rarely update the documentation when I change
 *		 something. Be warned...
 *****************************************************************************/

#include "BaoTree.h"
#include "BaoError.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*****************************************************************************
 * 								PRIVATE DATA								 *
 *****************************************************************************/
#define IN_NAMUA(state, player) (state->board[player][H_STORE])

#define IN_MTAJI(state, player) (state->board[player][H_STORE] == 0)

#define IN_CAPTURE_RANGE(hole) (hole >= H_LFKICHWA && hole <= H_RFKICHWA)

static Hole get_opposing_hole(Hole h)
{
	h = H_RFKICHWA - h;
	if(h < H_LFKICHWA)
		return H_STORE + h;
	return h;
}


static Player get_opponent(Player p)
{
	if(p == P_NORTH)
		return P_SOUTH;
	else 
		return P_NORTH;
}


/* An interface for the following test_*_capture functions */
static int can_capture(const BaoState *s, const BaoRules *r, Hole h)
{
	Player p = GET_PLAYER(s->flags);

	if(!IN_CAPTURE_RANGE(h) || s->board[p][h] == 0
	|| s->board[get_opponent(p)][get_opposing_hole(h)] == 0)
		return 0;
	return 1;
}



typedef int (*MoveTestFunc)(const BaoState *, const BaoRules *,
		Hole, MoveExecDir);


/***************************************************************************
 * test_namua_capture: Tests for a capture in namua stage.
 *
 *		Function checks if it is possible to play a namua capture move
 *		at hole h with an initial direction of d.
 *
 * Returns:	0 if move (h, d) is impossible else any other non-zero int.
 *
 * NOTE: Function assumes state is in NAMUA
 **************************************************************************/
static int test_namua_capture(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	return can_capture(state, rules, h);
}


/****************************************************************************
 * test_namua_takata: Tests for a takata in namua
 *
 *		Function tests for takata moves in namua stage that don't apply any
 *		obscure nyumba rules hence testing for a move at h == NYUMBA will
 *		always be false.
 *
 * Returns: Non-zero if move is defined else 0
 ***************************************************************************/
static int test_namua_takata(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	/* NOTE: During normal game play, a takata on NYUMBA is not allowed */
	if(state->board[state->player][h] == 0
	|| (h == H_NYUMBA && state->nyumba[state->player]))
		return 0;
	return 1;
}



static int test_namua_takata_singleton(const BaoState *state,
		const BaoRules *rules, Hole h, MoveExecDir d)
{
	if(state->board[state->player][h] == 0 || h == H_NYUMBA)
		return 0;
	return 1;
}


/****************************************************************************
 * test_namua_special: Test for the special namua takata
 *
 * 		This is just a convinience function for testing for the special
 *		takata in namua that starts on the nyumba by either lifting a single
 * 		nkhomo from the nyumba and store and sowing left or right of the
 * 		nyumba or lifting the whole nyumba when it is below a minimum
 * 		specified by the game's rules.
 *
 * Returns:	0 if move is not found else 1
 ****************************************************************************/
static int test_namua_special(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	if(state->nyumba[state->player] && state->board[state->player][H_NYUMBA])
		return 1;
	return 0;
}


/*****************************************************************************
 * test_mtaji_capture: Test for a capture in mtaji stage
 *
 * Returns: Non-zero if successful else 0
 *****************************************************************************/
static int test_mtaji_capture(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	if(!IN_CAPTURE_RANGE(h) || state->board[state->player][h] == 0
	|| state->board[state->player][h] > rules->max_nkhomo_for_mtaji_capture)
		return 0;

	if(d == MXD_RIGHT)
		h += state->board[state->player][h];
	else if(d == MXD_LEFT)
		h -= state->board[state->player][h];
	else
		return 0;
	return can_capture(state, rules, h % (H_LBKICHWA + 1));
}


/*****************************************************************************
 * test_mtaji_takata: Test for a takata move in mtaji stage
 *
 * Returns: Non-zero on success else 0
 *****************************************************************************/
static int test_mtaji_takata(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	if(state->board[state->player][h] <= 1) {
		return 0;
	} else if(rules->has_mtaji_moja_trap && state->trapped_hole == h) {
		if(h == H_NYUMBA && state->nyumba[state->player])
			return 1;		/* Nyumba can't be takasiad */
		return 0;
	}
	return 1;
}


/****************************************************************************
 * test_mtaji_special: Test for a mtaji special move
 *
 *		This function checks for the move that cancels out an opponent's
 *		mtaji moja trap. This occurs when a player has no other possible
 * 		takatas but the one blocked by the mtaji moja trap.
 *
 * Returns: 1 if it is possible, else return 0
 *
 * NOTE: h and d are not used, state->trapped_hole is used
 ***************************************************************************/
static int test_mtaji_special(const BaoState *state, const BaoRules *rules,
		Hole h, MoveExecDir d)
{
	Hole i;

	if(state->trapped_hole < H_RFKICHWA
	|| state->board[state->player][state->trapped_hole] <= 1)
		return 0;
	for(i = H_LFKICHWA; i <= H_RFKICHWA; i++) {
		/**********************************************************************
		 * mtaji special can be played only if there are no other moves
		 * to play.
		 *********************************************************************/
		if(i == state->trapped_hole)
			continue;
		if(state->board[state->player][i] > 1)
			return 0;
	}
	return 1;
}


/****************************************************************************
 * get_moves_bak: Fills buf with all valid moves in the range [start, end].
 *
 *		This function uses test_move to determine all acceptable moves in the
 *		range [start, end]. This function provides a backend to get_moves.
 *
 * Returns: Number of moves found.
 ***************************************************************************/
static int get_moves_bak(Move *buf, int bufsz, const BaoState *state,
		const BaoRules *rules, Hole start, Hole stop, MoveTestFunc test_move)
{
	int nmoves = 0;

	while(start <= stop) {
		if(test_move(state, rules, start, MXD_LEFT)) {
			if(nmoves == bufsz)
				return nmoves;
			buf[nmoves].hole = start;
			buf[nmoves].dir = MXD_LEFT;
			nmoves++;
		}
		if(test_move(state, rules, start, MXD_RIGHT)) {
			if(nmoves == bufsz)
				return nmoves;
			buf[nmoves].hole = start;
			buf[nmoves].dir = MXD_RIGHT;
			nmoves++;
		}
		start++;
	}
	return nmoves;
}


/*****************************************************************************
 * get_moves: Fill buf with up to n possible moves if available.
 *
 *		Function probes for moves acceptable to state. It determines the type
 *		of possible moves (takata or capture) and sets the takata flag if
 *		the moves found are takata else unsets it if only mtaji moves or
 *		no moves(imples gameOver) are found.
 *
 * Returns: Number of moves found
 ****************************************************************************/
static int get_moves(Move *buf, int bufsz, BaoState *state,
		const BaoRules *rules)
{
	int nmoves;

	if(IN_NAMUA(state, state->player)) {
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_LFKICHWA,
				H_RFKICHWA, test_namua_capture);
		if(nmoves)
			return nmoves;
		state->takata = 1;
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_LFKICHWA,
				H_RFKICHWA, test_namua_takata);
		if(nmoves)
			return nmoves;
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_LFKICHWA,
				H_RFKICHWA, test_namua_takata_singleton);
		if(nmoves)
			return nmoves;
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_NYUMBA, H_NYUMBA,
				test_namua_special);
		if(nmoves == 0)
			state->takata = 0;
		return nmoves;
	} else {
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_LFKICHWA,
				H_LBKICHWA, test_mtaji_capture);
		if(nmoves)
			return nmoves;
		state->takata = 1;
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_LFKICHWA,
				H_RFKICHWA, test_mtaji_takata);
		if(nmoves)
			return nmoves;
		nmoves = get_moves_bak(buf, bufsz, state, rules, state->trapped_hole,
				state->trapped_hole, test_mtaji_special);
		if(nmoves)
			return nmoves;
		nmoves = get_moves_bak(buf, bufsz, state, rules, H_RBKICHWA,
				H_LBKICHWA, test_mtaji_takata);
		if(nmoves == 0)
			state->takata = 0;
		return nmoves;
	}
}


/******************************************************************************
 * dup_node: Duplicates node's state and move.
 *
 *		The function creates a new tree and copies state and move from node.
 *
 * Returns: A node with the copied state on success else NULL
 *****************************************************************************/
static BaoTree *dup_node(const BaoTree *node)
{
	BaoTree *new_node;

	if((new_node = (BaoTree *) malloc(sizeof(BaoTree))) == NULL)
		return NULL;
	memcpy(&new_node->move, &node->move, sizeof(Move));
	memcpy(&new_node->state, &node->state, sizeof(BaoState));
	memset(new_node->children, 0, sizeof(BaoTree *) * MAXTRANS);
	return new_node;
}

/*****************************************************************************
 * get_mtaji_moja_trap: Probe for a mtaji moja trap on state
 *
 * Returns: Trapped hole if found else H_STORE
 *
 * NOTE: Trapping H_STORE is baologically impossible
 *****************************************************************************/
static Hole get_mtaji_moja_trap(const BaoState *state, const BaoRules *rules)
{
	Move buf[MAXTRANS];
	int nmoves, i;

	/* NYAXI... NYAXI.. NYAXI... NYAXI... */
	/* NOTE: Multiple moves may trap the same hole */
	nmoves = get_moves_bak(buf, MAXTRANS, state, rules, H_LFKICHWA, H_LBKICHWA,
					test_mtaji_capture);
	if(nmoves == 0)
		return -1;
	for(i = 1; i < nmoves; i++)
		if(buf[0].hole != buf[i].hole)
			return -1;
	return buf[0].hole;
	/* NYAXI... NYAXI.. NYAXI... NYAXI... */
}


/******************************************************************************
 * prep_state: Prepare state for the following/next player(current opponent)
 *
 *		Function update state's variables so as to facilitate the proper
 *		switching of players. trapped_hole is set if it's sane to do so,
 *		takata in unset and player is alternated (switched).
 *****************************************************************************/
static void prep_state(BaoState *state, const BaoRules *rules)
{
	if(IN_MTAJI(state, state->player) && state->takata
	&& rules->has_mtaji_moja_trap)
		state->trapped_hole = get_mtaji_moja_trap(state, rules);
	else
		state->trapped_hole = -1;
	state->takata = 0;
	state->player = get_opponent(state->player);
}


static void update_node(BaoTree *node, const Move *move,
		const BaoRules *rules, int nyumba_sown)
{
	prep_state(&node->state,  rules);
	node->move.hole = move->hole;
	node->move.dir = move->dir;
	node->move.nyumba_sown = nyumba_sown;
}


static int can_play_namua_special(const BaoState *s, const BaoRules *r,
		Hole h)
{
	return (s->takata && h == H_NYUMBA && s->nyumba[s->player]
			&& s->board[s->player][h] >= r->min_nkhomo_for_namua_special);
}


static void Hand_lift(Hand *h)
{
	h->nkhomo += h->state->board[h->side][h->hole];
	h->state->board[h->side][h->hole] = 0;
}


static int Hand_can_lift(const Hand *h, const BaoRules *r)
{
	return (h->state->board[h->side][h->hole] > 1);
}


static int Hand_can_switch_side(const Hand *h, const BaoRules *r)
{
	/* cmp 'can_capture', FIX THIS! */
	if(h->hole > H_RFKICHWA || h->state->takata
	|| h->state->board[h->side][h->hole] <= 1
	|| h->state->board[get_opponent(h->side)][get_opposing_hole(h->hole)] == 0)
		return 0;
	return 1;
}


static void Hand_sow(Hand *h)
{
	h->state->board[h->side][h->hole]++;
	h->nkhomo--;
}


static void Hand_switch_side(Hand *h)
{
	h->side = get_opponent(h->side);
	h->hole = get_opposing_hole(h->hole);
}


static void Hand_step(Hand *h)
{
	switch(h->dir) {
		case MXD_RIGHT:
			if(++(h->hole) > H_LBKICHWA)
				h->hole =H_LFKICHWA;
			break;
		case MXD_LEFT:
			if(--(h->hole) > H_LBKICHWA)
				h->hole = H_LBKICHWA;
			break;
	}
}


static void Hand_reset(Hand *h)
{
	if(h->hole <=H_LFKIMBI) {
		h->hole =H_LFKICHWA;
		h->dir = MXD_RIGHT;
	} else if(h->hole >= H_RFKIMBI) {
		h->hole = H_RFKICHWA;
		h->dir = MXD_LEFT;
	} else if(h->dir == MXD_RIGHT) {
		h->hole =H_LFKICHWA;
	} else { /* hand->dir == MXD_LEFT */
		h->hole = H_RFKICHWA;
	}
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


static int get_best_score(Player player, BaoTree *node,
		const BaoRules *rules, int depth)
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


/*****************************************************************************
 *								PUBLIC DATA									 *
 *****************************************************************************/


BaoTree *new_tree(const BaoRules *rules)
{
	BaoTree *top;
	Hole h;

	if((top = (BaoTree *) malloc(sizeof(BaoTree))) == NULL)
		return NULL;

	top->move.hole = H_STORE;
	top->move.dir = 0;
	top->move.nyumba_sown = 0;

	for(h =H_LFKICHWA; h <= H_STORE; h++) {
		top->state.board[P_NORTH][h] = rules->board_setting[h];
		top->state.board[P_SOUTH][h] = rules->board_setting[h];
	}
	top->state.nyumba[P_NORTH] = rules->has_nyumba;
	top->state.nyumba[P_SOUTH] = rules->has_nyumba;
	top->state.takata = 0;
	top->state.trapped_hole = H_STORE;
	top->state.player = P_SOUTH;

	top->parent = NULL;

	memset(top->children, 0, sizeof(BaoTree *) * MAXTRANS);

	top->nchildren = 0;

	return top;
}


void free_tree(BaoTree *top)
{
	BaoTree **p;

	for(p = top->children; *p != NULL; p++)
		free_tree(*p);
	free(top);
}



/*****************************************************************************
 *	grow_tree: Branch parent into the next possible states.
 *
 *		Function identifies possible (and acceptable) moves on parent's state
 * 		(parent->state), which it uses to populate parent->children with the
 * 		next possible sub trees (branches). All contents of parent->children
 * 		are overwritten with the new sub trees and a NULL marker in the end.
 *
 *	Returns: Number of sub trees generated else a negative value  on error.
 *****************************************************************************/
int grow_tree(BaoTree *parent, const BaoRules *rules)
{
	Move buf[MAXTRANS];		/* possible moves */
	Hand *hand;
	BaoTree *child;
	int i, nmoves;
	MoveExecSts exec_sts;

	if(parent->nchildren)
		return parent->nchildren;
	nmoves = get_moves(buf, MAXTRANS, &parent->state, rules);
	for(i = 0; i < nmoves; i++) {
		if((child = dup_node(parent)) == NULL)
			return -1;
		hand = start_move(&child->state, rules, &buf[i]);
		exec_sts = exec_move(hand, rules, rules->max_move_exec_depth);
		if(exec_sts == MXS_HAULTED) {
			if((parent->children[parent->nchildren] = dup_node(child)) == NULL)
				return -1;
			update_node(parent->children[parent->nchildren],
					&buf[i], rules, 1);
			parent->nchildren++;
			continue_move(hand);
			exec_sts = exec_move(hand, rules, rules->max_move_exec_depth);
		}
		if(exec_sts == MXS_NOTDONE) {
			/* Possible never ending move */
			end_move(hand);
			free_tree(child);
		} else {
			parent->children[parent->nchildren] = child;
			update_node(child, &buf[i], rules, 0);
			parent->nchildren++;
			end_move(hand);
		}
	}
	return parent->nchildren;
}


#define CMP_MOVE(m, n) \
	(((m).hole == (n).hole) && ((m).dir == (n).dir) \
	 && ((m).nyumba_sown == (n).nyumba_sown))

/*****************************************************************************
 * find_branch: Finds the branch move on node leads to
 *
 * 		Functions searchs node->children and to match them to move
 * 		(see BaoTree.move), if a possible match is found, it's path/index is
 * 		returned else -1 is returned.
 *
 * Returns:	Returns path/index of matching child else -1.
 ****************************************************************************/
int find_branch(BaoTree *node, const Move *move)
{
	int i;

	for(i = 0; i < node->nchildren; i++)
		if(CMP_MOVE(node->children[i]->move, *move))
			return i;
	return -1;
}


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


int shift_tree(BaoTree **node_p, uint path)
{
	if(path > (uint) (*node_p)->nchildren)
		return -1;
	*node_p = (*node_p)->children[path];
	return 0;
}


int unshift_tree(BaoTree **node_p)
{
	if((*node_p)->parent == NULL)
		return -1;
	*node_p = (*node_p)->parent;
	return 0;
}


Hand *start_move(BaoState *state, const BaoRules *rules, const Move *move)
{
	Hand *hand;

	if((hand = (Hand *) malloc(sizeof(Hand))) == NULL)
		return NULL;

	hand->state = state;
	hand->side = state->player;
	hand->hole = move->hole;
	hand->nkhomo = 0;
	hand->dir = move->dir;

	if(state->board[state->player][H_STORE]) {
		/* NAMUA requires a bit extra */
		state->board[state->player][H_STORE]--;
		state->board[state->player][move->hole]++;
		if(can_play_namua_special(state, rules, move->hole)) {
			state->board[state->player][H_NYUMBA]--;
			state->board[state->player][H_STORE]--;
			hand->nkhomo = 2;
		}
	} else {
		if(hand->state->takata == 0
		&& (hand->state->nyumba[P_NORTH] || hand->state->nyumba[P_SOUTH])) {
			hand->state->nyumba[P_NORTH] = 0;
			hand->state->nyumba[P_SOUTH] = 0;
		}
		Hand_lift(hand);
	}

	return hand;
}


int exec_move(Hand *hand, const BaoRules *rules, int steps)
{
	while(steps > 0) {
		if(hand->nkhomo == 0) {
			if(Hand_can_switch_side(hand, rules)) {	/* can capture */
				Hand_switch_side(hand);
				Hand_lift(hand);
				if(hand->hole == H_NYUMBA && hand->state->nyumba[hand->side])
					hand->state->nyumba[hand->side] = 0;
			} else if(Hand_can_lift(hand, rules)) {
				if(hand->hole == H_NYUMBA && hand->state->nyumba[hand->side]) {
					if(hand->state->board[hand->side][H_STORE]) {
						if(hand->state->takata)
							return MXS_DONE;
						return MXS_HAULTED;
					} else {
						Hand_lift(hand);
						hand->state->nyumba[hand->side] = 0;
						/* continue execution */
					}
				} else if(hand->state->board[hand->side][H_STORE] == 0
				       && hand->state->takata
				       && hand->hole == hand->state->trapped_hole
				       && rules->has_mtaji_moja_trap) {
					return MXS_DONE;
				} else {
					Hand_lift(hand);
					/* continue execution */
				}
			} else {
				return MXS_DONE;
			}
		} else  {
			if(hand->side != hand->state->player) {
				/* Just captured */
				Hand_switch_side(hand);
				Hand_reset(hand);
				Hand_sow(hand);
			} else {
				Hand_step(hand);
				Hand_sow(hand);
			}
		}
		steps--;
	}
	return MXS_NOTDONE;
}


void continue_move(Hand *hand)
{
	hand->state->nyumba[hand->side] = 0;
}


void end_move(Hand *hand)
{
	free(hand);
}

