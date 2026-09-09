/* Host-side perft for geochess-ai.h, with FEN setup and divide. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "geochess-ai.h"

static int fails = 0;

static int fen(const char *f)
{
    int sq = 0, i;
    int stm = CWHITE;
    const char *p = f;

    for (i = 0; i < 128; ++i) board[i] = 0;
    castle_rights = 0;
    ep_square = -1;

    for (; *p && *p != ' '; ++p) {
        if (*p == '/') { sq = (sq + 16) & 0x70; continue; }
        if (*p >= '1' && *p <= '8') { sq += *p - '0'; continue; }
        switch (*p) {
            case 'P': board[sq] = CWHITE | 1; break;
            case 'p': board[sq] = CBLACK | 2; break;
            case 'N': board[sq] = CWHITE | 4; break;
            case 'n': board[sq] = CBLACK | 4; break;
            case 'B': board[sq] = CWHITE | 5; break;
            case 'b': board[sq] = CBLACK | 5; break;
            case 'R': board[sq] = CWHITE | 6; break;
            case 'r': board[sq] = CBLACK | 6; break;
            case 'Q': board[sq] = CWHITE | 7; break;
            case 'q': board[sq] = CBLACK | 7; break;
            case 'K': board[sq] = CWHITE | 3; break;
            case 'k': board[sq] = CBLACK | 3; break;
            default: break;
        }
        ++sq;
    }

    while (*p == ' ') ++p;
    stm = (*p == 'b') ? CBLACK : CWHITE;
    while (*p && *p != ' ') ++p;
    while (*p == ' ') ++p;

    for (; *p && *p != ' '; ++p) {
        if (*p == 'K') castle_rights |= CR_WK;
        if (*p == 'Q') castle_rights |= CR_WQ;
        if (*p == 'k') castle_rights |= CR_BK;
        if (*p == 'q') castle_rights |= CR_BQ;
    }
    while (*p == ' ') ++p;

    if (*p && *p != '-')
        ep_square = (('8' - p[1]) * 16) + (p[0] - 'a');

    return stm;
}

static long perft(int color, int d)
{
    ChessIterator it; ChessMove mv; ChessUndo un;
    long n = 0;
    if (d == 0) return 1;
    ChessBegin(&it, 0);
    while (ChessNextLegal(color, &it, &mv)) {
        if (d == 1) { n++; continue; }
        ChessMake(&mv, &un);
        n += perft(24 - color, d - 1);
        ChessUnmake(&mv, &un);
    }
    return n;
}

static void divide(const char *f, int d)
{
    ChessIterator it; ChessMove mv; ChessUndo un;
    char t[5];
    int color = fen(f);
    ChessBegin(&it, 0);
    while (ChessNextLegal(color, &it, &mv)) {
        ChessMake(&mv, &un);
        ChessFormatMove(&mv, t);
        printf("   %s %ld\n", t, perft(24 - color, d - 1));
        ChessUnmake(&mv, &un);
    }
}

static void test(const char *name, const char *f, int d, long want)
{
    int color = fen(f);
    long got;
    int i;
    unsigned char snap[128];

    memcpy(snap, board, sizeof(snap));
    got = perft(color, d);

    printf("  %-22s d%d  %-10ld want %-10ld %s\n",
           name, d, got, want, got == want ? "OK" : "*** MISMATCH ***");
    if (got != want) { fails++; divide(f, d); }

    for (i = 0; i < 128; ++i)
        if (board[i] != snap[i]) {
            printf("  *** board not restored at square %d\n", i);
            fails++;
            break;
        }
}

int main(void)
{
    const char *START =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    /* Kiwipete: castling and en passant in volume */
    const char *KIWI =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    /* Position 3: rook and pawn endgame, en passant, no castling */
    const char *POS3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";

    printf("Start position (en passant appears at depth 5):\n");
    test("startpos", START, 1, 20);
    test("startpos", START, 2, 400);
    test("startpos", START, 3, 8902);
    test("startpos", START, 4, 197281);
    test("startpos", START, 5, 4865609);

    printf("\nKiwipete (castling-heavy):\n");
    test("kiwipete", KIWI, 1, 48);
    test("kiwipete", KIWI, 2, 2039);
    test("kiwipete", KIWI, 3, 97862);
    /*
     * Reference is 4085603. This engine promotes to a queen only, so
     * each of the 15172 reference promotion nodes collapses to one:
     * 4085603 - (15172 - 15172/4) = 4074224. Documented limitation,
     * not a castling or en passant defect.
     */
    test("kiwipete", KIWI, 4, 4074224);

    printf("\nPosition 3 (en passant, no castling):\n");
    test("position3", POS3, 1, 14);
    test("position3", POS3, 2, 191);
    test("position3", POS3, 3, 2812);
    test("position3", POS3, 4, 43238);

    printf("\n%d failure(s)\n", fails);
    return fails != 0;
}
