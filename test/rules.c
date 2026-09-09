/* Castling and en passant rule tests for geochess-ai.h */
#include <stdio.h>
#include <string.h>
#include "geochess-ai.h"

#define SQ(f,r) (((8-(r))*16)+((f)-'a'))
static int fails = 0;

static void clear(void)
{
    int i;
    for (i = 0; i < 128; ++i) board[i] = 0;
    castle_rights = CR_ALL;
    ep_square = -1;
}

static void put(const char *s, int p) { board[SQ(s[0], s[1]-'0')] = p; }

static void chk(const char *name, int got, int want)
{
    printf("  %-52s %s\n", name, got == want ? "OK" : "*** FAIL ***");
    if (got != want) { fails++; printf("      got %d want %d\n", got, want); }
}

/* both sides on their home squares, nothing in between */
static void home(void)
{
    clear();
    put("e1", CWHITE|3); put("a1", CWHITE|6); put("h1", CWHITE|6);
    put("e8", CBLACK|3); put("a8", CBLACK|6); put("h8", CBLACK|6);
}

static int legal(int c, const char *a, const char *b)
{
    return ChessLegal(c, SQ(a[0],a[1]-'0'), SQ(b[0],b[1]-'0'));
}

static void play(int c, const char *a, const char *b)
{
    ChessMove m; ChessUndo u;
    m.from = SQ(a[0],a[1]-'0'); m.to = SQ(b[0],b[1]-'0');
    (void)c;
    ChessMake(&m, &u);
}

int main(void)
{
    ChessMove m; ChessUndo u;
    int i, rsnap, esnap;
    unsigned char snap[128];

    printf("Castling:\n");

    home();
    chk("white kingside legal from home position", legal(CWHITE,"e1","g1"), 1);
    chk("white queenside legal from home position", legal(CWHITE,"e1","c1"), 1);
    chk("black kingside legal from home position", legal(CBLACK,"e8","g8"), 1);
    chk("black queenside legal from home position", legal(CBLACK,"e8","c8"), 1);

    home(); put("f1", CWHITE|4);
    chk("blocked by own knight on f1", legal(CWHITE,"e1","g1"), 0);
    home(); put("b1", CWHITE|4);
    chk("blocked by own knight on b1", legal(CWHITE,"e1","c1"), 0);

    home(); put("e5", CBLACK|6);
    chk("cannot castle out of check", legal(CWHITE,"e1","g1"), 0);
    home(); put("f5", CBLACK|6);
    chk("cannot castle through an attacked square", legal(CWHITE,"e1","g1"), 0);
    home(); put("g5", CBLACK|6);
    chk("cannot castle onto an attacked square", legal(CWHITE,"e1","g1"), 0);

    home(); put("h5", CBLACK|6);
    chk("may castle while the rook is attacked", legal(CWHITE,"e1","g1"), 1);
    home(); put("b5", CBLACK|6);
    chk("queenside legal though b1 is attacked", legal(CWHITE,"e1","c1"), 1);

    /* rights are withdrawn and never restored by a return trip */
    home();
    play(CWHITE, "e1", "e2");
    chk("king move clears both white rights",
        castle_rights & (CR_WK|CR_WQ), 0);
    chk("black rights untouched by a white king move",
        castle_rights & (CR_BK|CR_BQ), CR_BK|CR_BQ);

    home();
    play(CWHITE, "h1", "h2");
    chk("h1 rook move clears kingside only",
        castle_rights & (CR_WK|CR_WQ), CR_WQ);

    home();
    play(CWHITE, "a1", "a2");
    chk("a1 rook move clears queenside only",
        castle_rights & (CR_WK|CR_WQ), CR_WK);

    /* the classic case: a rook captured where it stands */
    home(); put("h8", 0); put("h7", CBLACK|6);
    play(CBLACK, "h7", "h1");
    chk("rook captured on h1 clears white kingside",
        castle_rights & CR_WK, 0);
    chk("white queenside survives that capture",
        castle_rights & CR_WQ, CR_WQ);

    /* make/unmake must restore board, rights and ep target exactly */
    home();
    memcpy(snap, board, sizeof(snap));
    rsnap = castle_rights; esnap = ep_square;
    m.from = SQ('e',1); m.to = SQ('g',1);
    ChessMake(&m, &u);
    chk("castling places the king on g1", board[SQ('g',1)], CWHITE|3);
    chk("castling places the rook on f1", board[SQ('f',1)], CWHITE|6);
    chk("castling empties e1", board[SQ('e',1)], 0);
    chk("castling empties h1", board[SQ('h',1)], 0);
    ChessUnmake(&m, &u);
    for (i = 0; i < 128; ++i)
        if (board[i] != snap[i]) break;
    chk("unmake restores every square", i, 128);
    chk("unmake restores castling rights", castle_rights, rsnap);
    chk("unmake restores the en passant target", ep_square, esnap);

    printf("\nEn passant:\n");

    /* white double push exposes e3 to a black pawn on d4 */
    clear();
    put("e2", CWHITE|1); put("d4", CBLACK|2);
    put("e1", CWHITE|3); put("h8", CBLACK|3);
    play(CWHITE, "e2", "e4");
    chk("double push sets the target square", ep_square, SQ('e',3));
    chk("black may capture en passant", legal(CBLACK,"d4","e3"), 1);

    m.from = SQ('d',4); m.to = SQ('e',3);
    memcpy(snap, board, sizeof(snap));
    rsnap = castle_rights; esnap = ep_square;
    ChessMake(&m, &u);
    chk("capturing pawn lands on e3", board[SQ('e',3)], CBLACK|2);
    chk("the captured pawn is removed from e4", board[SQ('e',4)], 0);
    chk("d4 is vacated", board[SQ('d',4)], 0);
    ChessUnmake(&m, &u);
    for (i = 0; i < 128; ++i)
        if (board[i] != snap[i]) break;
    chk("unmake restores the captured pawn", i, 128);
    chk("unmake restores the en passant target", ep_square, esnap);
    chk("unmake restores rights", castle_rights, rsnap);

    /* the right expires after one move */
    clear();
    put("e2", CWHITE|1); put("d4", CBLACK|2);
    put("e1", CWHITE|3); put("h8", CBLACK|3); put("a8", CBLACK|6);
    play(CWHITE, "e2", "e4");
    play(CBLACK, "a8", "b8");
    chk("target is cleared by an unrelated move", ep_square, -1);
    chk("en passant is no longer available", legal(CBLACK,"d4","e3"), 0);

    /* a single push does not expose anything */
    clear();
    put("e2", CWHITE|1); put("d4", CBLACK|2);
    put("e1", CWHITE|3); put("h8", CBLACK|3);
    play(CWHITE, "e2", "e3");
    chk("single push sets no target", ep_square, -1);

    /* black double push, mirrored */
    clear();
    put("d7", CBLACK|2); put("e5", CWHITE|1);
    put("e1", CWHITE|3); put("h8", CBLACK|3);
    play(CBLACK, "d7", "d5");
    chk("black double push sets the target", ep_square, SQ('d',6));
    chk("white may capture en passant", legal(CWHITE,"e5","d6"), 1);
    m.from = SQ('e',5); m.to = SQ('d',6);
    ChessMake(&m, &u);
    chk("black pawn removed from d5", board[SQ('d',5)], 0);
    ChessUnmake(&m, &u);
    chk("black pawn restored on d5", board[SQ('d',5)], CBLACK|2);

    /*
     * The horizontal pin. An en passant capture removes two pawns
     * from the same rank at once, so it can open a line onto the
     * capturing side's own king even though neither pawn was pinned.
     * Position: white Ka5 and Pe5, black rook h5, black plays d7-d5.
     */
    clear();
    put("a5", CWHITE|3); put("e5", CWHITE|1);
    put("d7", CBLACK|2); put("h5", CBLACK|6); put("h8", CBLACK|3);
    play(CBLACK, "d7", "d5");
    chk("target set by the double push", ep_square, SQ('d',6));
    chk("en passant that opens a rank onto own king is rejected",
        legal(CWHITE,"e5","d6"), 0);

    /* the same position with the rank already blocked is fine */
    clear();
    put("a5", CWHITE|3); put("e5", CWHITE|1);
    put("d7", CBLACK|2); put("h5", CBLACK|6); put("h8", CBLACK|3);
    put("b5", CWHITE|4);            /* knight closes the rank */
    play(CBLACK, "d7", "d5");
    chk("same capture is legal once the rank is blocked",
        legal(CWHITE,"e5","d6"), 1);

    printf("\n%d failure(s)\n", fails);
    return fails != 0;
}
