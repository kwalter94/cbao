#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


BaoRules rules[] = {
	{
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 22}, 0, 1, 16, 0, 50
	},
	{
		{0, 0, 0, 0, 8, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20}, 1, 1, 16, 8, 50
	},
	{
		{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0}, 0, 1, 16, 0, 50
	}
};


void print_state(BaoState *s)
{
	Hole h;

	printf("\tN: ");
	for(h = H_LFKICHWA; h <= H_STORE; h++)
		printf("%d, ", s->board[P_NORTH][h]);
	printf("\n");
	printf("\tS: ");
	for(h = H_LFKICHWA; h <= H_STORE; h++)
		printf("%d, ", s->board[P_SOUTH][h]);
	printf("\n");

	printf("nyumba: N %d S %d\n", s->nyumba[P_NORTH], s->nyumba[P_SOUTH]);
	printf("takata: %d\n", s->takata);
	printf("trapped_hole: %d\n", s->trapped_hole);
	printf("player: %c\n", s->player == P_NORTH ? 'N': 'S');
}


void print_move(Move *m)
{
	printf("%d-%s ", m->hole, m->dir == MXD_RIGHT ? "CK": "ANTCK");
}


void print_node(BaoTree *n)
{
	int i;

	printf("Move: ");
	print_move(&(n->move));
	printf("\n");
	printf("State:\n");
	print_state(&(n->state));
	printf("\n");
	printf("Moves: ");
	for(i = 0; n->children[i] != NULL; i++) {
		printf("%d.", i + 1);
		print_move(&(n->children[i]->move));
	}
	printf("\n");
}


int main(int argc, char *argv[])
{
	BaoTree *tree;
	BaoTree **child_p;
	Hand *hand;
	char line[80];
	int i;

	if((tree = new_tree(&rules[1])) == NULL) {
		perror("Could not initialise a new game");
		exit(EXIT_FAILURE);
	}

	if(grow_tree(tree, &rules[1]) == -1) {
		perror("Could not update tree");
		exit(EXIT_FAILURE);
	}

	printf("%d child nodes found\n", tree->nchildren);
	print_node(tree);

	printf("---------BEGIN-CHILDREN----------\n");
	for(child_p = tree->children; *child_p != NULL; child_p++) {
		print_node(*child_p);
		printf("\n");
	}
	printf("----------END-CHILDREN-----------\n");


	hand = NULL;
	i = best_branch(tree, &rules[1], 5);
	printf("Best branch: %d\n", i);
	i = 0;
	print_node(tree);
	printf("> ");
	while(scanf("%79s", line) != EOF) {
		// line[strlen(line) - 1] = '\0'; 	/* strip \n */
		if(strcmp(line, "") == 0) {
			if(hand != NULL) {
				if(exec_move(hand, &rules[1], 1) == MXS_DONE) {
					printf("Move completed\n");
					end_move(hand);
					hand = NULL;
				}
			} else {
				continue;
			}
		} else if(strcmp(line, "next") == 0 || strcmp(line, "n") == 0) {
			if(hand == NULL) {
				printf("Error: No move in eval\n");
			} else if(exec_move(hand, &rules[1], 1) == MXS_DONE) {
				printf("Move completed\n");
				end_move(hand);
				hand = NULL;
			}
		} else if(isdigit(line[0])) {
			i = atoi(line) - 1;
			if(i < 0 || i > tree->nchildren)
				printf("Error: Invalid move index: %d\n", i + 1);
			printf("You are playing %d: ", i + 1);
			print_move(&tree->children[i]->move);
			printf("\n");
			hand = start_move(&tree->state, &rules[1],
					&tree->children[i]->move);
		} else {
			printf("unknown command %s\n", line);
		}
		print_node(tree);
		printf("> ");
	}

	exit(EXIT_SUCCESS);
}
