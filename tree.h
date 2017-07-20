#ifndef BAOTREE_H
#define BAOTREE_H


enum {
	NPLAYERS = 2,	/* Number of player's per game */
	NHOLES   = 17,	/* Number of holes owned by each player */
	MAXTRANS = 20	/* Max. # of transitions allowed per BaoState */
};


enum Hole {
	/* Special holes: */
	H_LBKICHWA  = 15,	/* Left back kichwa (b1/B1) */
	H_LBKIMBI   = 14,	/* Left back kimbi (b2/B2) */
	H_LFKICHWA  = 0,
	H_LFKIMBI   = 1,
	H_NYUMBA    = 4,
	H_RBKICHWA  = 8,
	H_RBKIMBI   = 9,
	H_RFKICHWA  = 7,
	H_RFKIMBI   = 6,
	H_STORE     = 16	/* Mwa nkhomo */
};


enum Player {
	P_NORTH = 0,
	P_SOUTH = 1
};

enum BaoStateFlag {
	F_PLAYER  = 0x01,
	F_NYUMBA_N = 0x02,
	F_NYUMBA_S = 0x04,
	F_TAKATA  = 0x08,
	F_TRAP    = 0xF0
};

#define GET_PLAYER(f) (f & F_PLAYER)

#define SET_PLAYER(f, p) (f |= p)

#define GET_NORTH_NYUMBA(f) (f & F_N_NYUMBA)

#define SET_NORTH_NYUMBA(f) (f |= F_N_NYUMBA)

#define UNSET_NORTH_NYUMBA(f) (f &= ~F_N_NYUMBA)

#define GET_SOUTH_NYUMBA(f) (f & F_S_NYUMBA)

#define SET_SOUTH_NYUMBA(f) (f |= F_S_NYUMBA)

#define UNSET_SOUTH_NYUMBA(f) (f &= ~F_S_NYUMBA)

#define GET_TAKATA(f) (f & F_TAKATA)

#define SET_TAKATA(f) (f |= F_TAKATA)

#define UNSET_TAKATA(f) (f &= ~F_TAKATA)

#define GET_TRAPPED_HOLE(f) ((f & F_TRAP) >> 4)

#define SET_TRAPPED_HOLE(f, h) (f &= ~F_TRAP; f |= (byte) h << 4)


enum MoveExecDir {
	/* Move execution directions */
	MXD_LEFT  = -1,
	MXD_RIGHT = 1
};


enum MoveExecSts{
	/* Move execution status:
	 * 		These flags are returned after each step in a move.
	 *		They indicate the move's current state */
	MXS_ERROR,
	/* Met an undefined condition during execution */
	MXS_HAULTED,
	/* Hault's on first attempt to lift nyumba in a capture move */
	MXS_DONE,
	/* DONE EXECUTING THE MOVE! */
	MXS_NOTDONE
};


typedef enum Hole Hole;

typedef enum Flag Flag;

typedef enum MoveExecDir MoveExecDir;

typedef enum MoveExecSts MoveExecSts;

typedef enum Player Player;


struct BaoState {
	unsigned int board[NPLAYERS][NHOLES];
	unsigned char flags;
	int takata;
	int nyumba[NPLAYERS];
	Hole trapped_hole;
	Player player;
};




struct Move {
	Hole hole;				 /* Move exec start's here */

	MoveExecDir dir; /* Move exec direction (MXD_LEFT or MXD_RIGHT) */

	int nyumba_sown;
};




struct BaoTree {

	struct Move move;	/* Move that leads to current state. */

	struct BaoState state;

	struct BaoTree *parent;

	struct BaoTree *children[MAXTRANS];

	unsigned int nchildren;
};




struct BaoRules {
	/*  This defines a sub set of the bao rules that define the differences
	 * in how the game is played in different regions. */

	unsigned int board_setting[NHOLES];
	/* This is the layout of nkhomo on both player's sides of the board
	 * at the start of the game. */

	int has_nyumba;
	/* This defines the state of both player's nyumbas at the start of
	 * a game. */

	int has_mtaji_moja_trap;
	/* This defines whether the mtaji moja rule must be enforced during game
	 * play. */

	int max_nkhomo_for_mtaji_capture;
	/* This is the maximum nkhomo that a player can lift from some hole to start
	 * a capture move during the mtaji stage. */

	int min_nkhomo_for_namua_special;
	/* This is the minimum number of nkhomo that must be on nyumba to perform
	 * the special nyumba takata (sow one on nyumba, lift two from nyumba then
	 * takata) in namua stage. */

	int max_move_exec_depth;
	/* This is the maximum number of steps allowed in a move. Any move that
	 * exceeds this is assumed to be a perpertual move hence deemed illegal. */
};





struct Hand {
	/* This represents a player's hand during move execution. Move execution
	 * comprises sequential lifts and sows which add and reduce the nkhomo in
	 * the player's hand respectively. */

	struct BaoState *state;

	Player side;
	/* Side of board (state->board) player's (state->player) hand is
	 * currently positioned */

	Hole hole;
	/* This pairs with side to give the actual position of player's hand on
	 * the board */

	unsigned int nkhomo;
	/* Number of nkhomo (seeds) left in player's hand usable in subsequent
	 * sowings */

	MoveExecDir dir;
	/* The direction in which the player is sowing towards */
};



typedef struct BaoRules BaoRules;

typedef struct BaoState BaoState;

typedef struct BaoTree BaoTree;

typedef struct Hand Hand;

typedef struct Move Move;



BaoTree *new_tree(const BaoRules *rules);


void free_tree(BaoTree *tree);


int grow_tree(BaoTree *node, const BaoRules *rules);


int find_branch(BaoTree *node, const Move *move);


int shift_tree(BaoTree **node_p, unsigned int path);


int unshift_tree(BaoTree **node_p);


Hand *start_move(BaoState *state, const BaoRules *rules, const Move *move);


int exec_move(Hand *hand, const BaoRules *rules, int steps);


void continue_move(Hand *hand);


void end_move(Hand *hand);


#endif /* BAOTREE_H */
