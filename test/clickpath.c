/*
 * Drive the engine exactly the way HandleBoardClick() does: build a
 * move from board row/column, format it into user_move, commit with
 * playerMove(). Then confirm the front end can find every square that
 * changed by diffing the board, which is what RefreshBoardDisplay()
 * now relies on.
 */
#include <stdio.h>
#include <string.h>
#include "geochess-ai.h"

#define SQ(f,r) (((8-(r))*16)+((f)-'a'))
static int fails = 0;

static unsigned char before[128];

static void chk(const char *n, int got, int want)
{
    printf("  %-50s %s\n", n, got == want ? "OK" : "*** FAIL ***");
    if (got != want) { fails++; printf("      got %d want %d\n", got, want); }
}

/* the click path: two squares in, one committed move out */
static unsigned char click(unsigned char fr, unsigned char fc,
                           unsigned char tr, unsigned char tc)
{
    ChessMove move;
    move.from = (int)fr * 16 + fc;
    move.to = (int)tr * 16 + tc;
    ChessFormatMove(&move, user_move);
    memcpy(before, board, sizeof(before));
    return playerMove();
}

static int changed_squares(void)
{
    int r, c, n = 0;
    for (r = 0; r < 8; ++r)
        for (c = 0; c < 8; ++c)
            if (board[r * 16 + c] != before[r * 16 + c]) n++;
    return n;
}

int main(void)
{
    printf("User click path:\n");

    /* clear the white king side, then castle by clicking e1 then g1 */
    engine_init();
    board[SQ('f',1)] = 0;
    board[SQ('g',1)] = 0;

    chk("clicking e1 then g1 castles", click(7, 4, 7, 6), 1);
    chk("king reached g1", board[SQ('g',1)], CWHITE|3);
    chk("rook reached f1", board[SQ('f',1)], CWHITE|6);
    chk("front end sees four changed squares", changed_squares(), 4);
    chk("side passed to black", side, CBLACK);
    chk("move text recorded as e1g1", strcmp(user_move, "e1g1"), 0);

    /* queenside, same path */
    engine_init();
    board[SQ('b',1)] = 0;
    board[SQ('c',1)] = 0;
    board[SQ('d',1)] = 0;
    chk("clicking e1 then c1 castles queenside", click(7, 4, 7, 2), 1);
    chk("king reached c1", board[SQ('c',1)], CWHITE|3);
    chk("rook reached d1", board[SQ('d',1)], CWHITE|6);
    chk("front end sees four changed squares", changed_squares(), 4);

    /* clicking the rook instead of the destination is refused */
    engine_init();
    board[SQ('f',1)] = 0;
    board[SQ('g',1)] = 0;
    chk("clicking e1 then h1 is rejected", click(7, 4, 7, 7), 0);
    chk("board untouched by the rejected click", changed_squares(), 0);
    chk("still white to move", side, CWHITE);

    /* en passant through the same path */
    engine_init();
    {
        int i;
        for (i = 0; i < 128; ++i) if (!(i & 0x88)) board[i] = 0;
        board[SQ('e',1)] = CWHITE|3;
        board[SQ('e',8)] = CBLACK|3;
        board[SQ('e',2)] = CWHITE|1;
        board[SQ('d',4)] = CBLACK|2;
        castle_rights = 0;
        ep_square = -1;
        side = CWHITE;
        engine_state = ENGINE_PLAY;
    }
    chk("white double-pushes e2-e4", click(6, 4, 4, 4), 1);
    chk("target square exposed", ep_square, SQ('e',3));

    /* black is the engine side, so drive the capture directly */
    {
        ChessMove m; ChessUndo u;
        m.from = SQ('d',4); m.to = SQ('e',3);
        memcpy(before, board, sizeof(before));
        ChessMake(&m, &u);
        chk("en passant clears three squares", changed_squares(), 3);
        chk("capturing pawn on e3", board[SQ('e',3)], CBLACK|2);
        chk("captured pawn gone from e4", board[SQ('e',4)], 0);
        chk("d4 vacated", board[SQ('d',4)], 0);
    }

    printf("\n%d failure(s)\n", fails);
    return fails != 0;
}
