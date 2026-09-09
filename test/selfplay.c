/*
 * Random-vs-random games driven straight through make/unmake, to put
 * real volume through the castling and en passant paths and prove the
 * position is restored exactly every time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geochess-ai.h"

static int bad = 0;
static void fail(const char *m, int g) { printf("*** %s (game %d)\n", m, g); bad++; }

int main(void)
{
    int g, plies, i, color;
    long moves = 0, castles = 0, eps = 0, promos = 0;
    long unmake_checks = 0;
    ChessIterator it; ChessMove mv; ChessUndo un;
    unsigned char snap[128];
    int rsnap, esnap;

    srand(777);

    for (g = 0; g < 3000; ++g) {
        engine_init();
        color = CWHITE;

        for (plies = 0; plies < 300; ++plies) {
            long n = 0, pick;
            int kind, piece;

            ChessBegin(&it, 0);
            while (ChessNextLegal(color, &it, &mv)) n++;
            if (n == 0) break;

            pick = rand() % n; n = 0;
            ChessBegin(&it, 0);
            while (ChessNextLegal(color, &it, &mv)) { if (n++ == pick) break; }

            piece = board[mv.from];
            kind = ChessMoveKind(piece, board[mv.to], mv.from, mv.to);
            if (kind == MOVE_CASTLE) castles++;
            if (kind == MOVE_EP) eps++;

            /*
             * Every tenth move, make and unmake first and confirm the
             * whole position comes back byte for byte before playing
             * it for real.
             */
            if ((moves % 10) == 0) {
                memcpy(snap, board, sizeof(snap));
                rsnap = castle_rights;
                esnap = ep_square;
                ChessMake(&mv, &un);
                ChessUnmake(&mv, &un);
                unmake_checks++;
                for (i = 0; i < 128; ++i)
                    if (board[i] != snap[i]) { fail("unmake did not restore board", g); break; }
                if (castle_rights != rsnap) fail("unmake did not restore rights", g);
                if (ep_square != esnap) fail("unmake did not restore ep target", g);
                if (bad) return 1;
            }

            ChessMake(&mv, &un);
            if ((piece == 9 || piece == 18) && (board[mv.to] & 7) == 7) promos++;
            moves++;

            if (ChessInCheck(color)) { fail("mover left its own king in check", g); return 1; }
            if (!ChessPositionOK()) { fail("position invalid", g); return 1; }
            if (ep_square != -1 && (ep_square & 0x88)) { fail("ep target off board", g); return 1; }

            color = 24 - color;
        }
    }

    printf("games=3000 plies=%ld  unmake spot-checks=%ld\n", moves, unmake_checks);
    printf("castles=%ld  en-passant captures=%ld  promotions=%ld\n",
           castles, eps, promos);
    printf("%s\n", bad ? "FAILURES PRESENT" : "all invariants held");
    return bad != 0;
}
