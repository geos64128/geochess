/*
 * Randomized self-play through the public engine API, checking the
 * invariants the GEOS front end relies on.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geochess-ai.h"

static int bad = 0;
static void fail(const char *m, int g) { printf("*** %s (game %d)\n", m, g); bad++; }

static unsigned char offsnap[128];

int main(void)
{
    int g, plies, i;
    long games = 0, moves = 0, mates = 0, stales = 0;
    long castles = 0, eps = 0, promos = 0;
    ChessIterator it; ChessMove mv;

    engine_init();
    memcpy(offsnap, board, sizeof(board));
    srand(20240909);

    for (g = 0; g < 400; ++g) {
        engine_init();
        games++;

        for (plies = 0; plies < 200; ++plies) {
            long n = 0, pick;
            int before_from, before_to, kind;

            if (engine_state != ENGINE_PLAY) break;

            /* choose a move, then classify it before it is committed */
            if (side == CWHITE) {
                ChessBegin(&it, 0);
                while (ChessNextLegal(CWHITE, &it, &mv)) n++;
                if (n == 0) { fail("no legal move but state==PLAY", g); break; }
                pick = rand() % n; n = 0;
                ChessBegin(&it, 0);
                while (ChessNextLegal(CWHITE, &it, &mv)) { if (n++ == pick) break; }
                ChessFormatMove(&mv, user_move);
                before_from = board[mv.from];
                before_to = board[mv.to];
                kind = ChessMoveKind(before_from, before_to, mv.from, mv.to);
                if (!playerMove()) { fail("playerMove rejected a legal move", g); break; }
            } else {
                int src, dst;
                if (!aiMove()) {
                    if (engine_state == ENGINE_PLAY) fail("aiMove failed while playable", g);
                    break;
                }
                if (ai_move[0] == '\0') { fail("aiMove produced no text", g); break; }
                src = (xlateRow(ai_move[1]) * 16) + xlateCol(ai_move[0]);
                dst = (xlateRow(ai_move[3]) * 16) + xlateCol(ai_move[2]);
                before_from = board[dst];   /* piece has already moved */
                before_to = 0;
                kind = -1;
                (void)src;
            }

            if (kind == MOVE_CASTLE) castles++;
            if (kind == MOVE_EP) eps++;
            if (kind >= 0 && (before_from == 9 || before_from == 18) &&
                (board[mv.to] & 7) == 7) promos++;

            moves++;

            if (!ChessPositionOK()) { fail("position invalid after commit", g); break; }
            if (ChessInCheck(24 - side)) { fail("mover left its own king in check", g); break; }
            if (engine_state != ChessStatus(side)) { fail("engine_state disagrees with ChessStatus", g); break; }
            if (ep_square != -1 && (ep_square & 0x88)) { fail("ep target off board", g); break; }
            if (castle_rights & ~CR_ALL) { fail("stray castling-rights bits", g); break; }

            for (i = 0; i < 128; ++i)
                if ((i & 0x88) && board[i] != offsnap[i]) {
                    fail("off-board square written", g);
                    i = 1000;
                    break;
                }
            if (i == 1001) break;

            /* rights must never be granted back */
            if ((castle_rights & CR_WK) &&
                (board[SQ_E1] != (CWHITE|3) || board[SQ_H1] != (CWHITE|6))) {
                fail("kingside right held without king and rook at home", g); break;
            }
            if ((castle_rights & CR_BQ) &&
                (board[SQ_E8] != (CBLACK|3) || board[SQ_A8] != (CBLACK|6))) {
                fail("black queenside right held without king and rook at home", g); break;
            }
        }

        if (engine_state == ENGINE_MATE) mates++;
        else if (engine_state == ENGINE_STALEMATE) stales++;
        else if (engine_state == ENGINE_INVALID) fail("game ended ENGINE_INVALID", g);
    }

    printf("games=%ld plies=%ld mates=%ld stalemates=%ld\n", games, moves, mates, stales);
    printf("white castled %ld times, captured en passant %ld times, promoted %ld times\n",
           castles, eps, promos);
    printf("%s\n", bad ? "FAILURES PRESENT" : "all invariants held");
    return bad != 0;
}
