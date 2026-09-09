/*
 * GEOCHESS
 *
 * Original GEOS application:
 *   Scott Hutter, November 2023.
 *
 * You are free to modify this code as desired, as long as original
 * author credit is mentioned for both the GEOS code and the
 * included AI engines.
 *
 * Engine derived from:
 *
 *   BMCP v1.0
 *   by Maksim Korzh
 *   A tribute to the chess programming community.
 *   Based on ideas taken from micro-Max by H.G. Muller.
 *
 * Copyright (C) 2018 Maksim Korzh
 * <freesoft.for.people@gmail.com>
 *
 * This work is free. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License,
 * Version 2, as published by Sam Hocevar.
 *
 * THIS PROGRAM IS FREE SOFTWARE. IT COMES WITHOUT ANY WARRANTY,
 * TO THE EXTENT PERMITTED BY APPLICABLE LAW.
 *
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                     Version 2, December 2004
 *
 * Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *             DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *    TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *   0. You just DO WHAT THE FUCK YOU WANT TO.
 *
 * Corrected source revision: GEOCHESS_FIXSET_1.
 *
 * The original board encoding, movement-offset table and evaluation
 * weights are retained. Move legality, make/unmake, promotion,
 * terminal-state handling and root-move selection are reworked.
 *
 * Supported rules:
 *   Ordinary moves, castling, en passant and automatic queen
 *   promotion.
 *
 * Not implemented:
 *   Underpromotion, repetition, move-count draws or
 *   insufficient-material adjudication.
 *
 * Checkmate/stalemate classification uses this supported move set.
 *
 * IMPORTANT:
 *   This header contains definitions. Include it in one translation
 *   unit only, as in the original project.
 *
 *   Recursive search requires stack-based automatic variables.
 *   Do not compile with cc65 --static-locals / -Cl.
 *
 *   The engine is not reentrant.
 */

#ifndef GEOCHESS_AI_H
#define GEOCHESS_AI_H

#include <string.h>

#define CWHITE 8
#define CBLACK 16

#define ENGINE_PLAY       0
#define ENGINE_MATE       1
#define ENGINE_STALEMATE  2
#define ENGINE_INVALID    3

/* Castling rights, one bit per side and wing. */
#define CR_WK             1
#define CR_WQ             2
#define CR_BK             4
#define CR_BQ             8
#define CR_ALL            15

/* Home squares in the 0x88 layout. Row zero is rank eight. */
#define SQ_A8             0
#define SQ_E8             4
#define SQ_H8             7
#define SQ_A1             112
#define SQ_E1             116
#define SQ_H1             119

/* Move classifications, derived from the position and not stored. */
#define MOVE_PLAIN        0
#define MOVE_EP           1
#define MOVE_CASTLE       2

#define CHESS_INFINITY    30000
#define CHESS_MATE_SCORE  29000
#define CHESS_MAX_DEPTH   3

/*
 * Original 0x88 board plus positional scores.
 *
 * Each row occupies 16 entries:
 *   first eight entries: pieces;
 *   next eight entries: positional weights.
 *
 * Piece encoding:
 *   color: white=8, black=16;
 *   type: white pawn=1, black pawn=2, king=3, knight=4,
 *         bishop=5, rook=6, queen=7.
 */
static const unsigned char starting_board[128] = {
    22, 20, 21, 23, 19, 21, 20, 22,
     0,  0,  5,  5,  0,  0,  5,  0,

    18, 18, 18, 18, 18, 18, 18, 18,
     5,  5,  0,  0,  0,  0,  5,  5,

     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 15, 20, 20, 15, 10,  5,

     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 20, 30, 30, 20, 10,  5,

     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 20, 30, 30, 20, 10,  5,

     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 15, 20, 20, 15, 10,  5,

     9,  9,  9,  9,  9,  9,  9,  9,
     5,  5,  0,  0,  0,  0,  5,  5,

    14, 12, 13, 15, 11, 13, 12, 14,
     0,  0,  5,  5,  0,  0,  5,  0
};

unsigned char board[128];

/* Offsets span -33..33, so a signed byte each is enough. */
static const signed char move_offsets[] = {
     15,  16,  17,   0,
    -15, -16, -17,   0,
      1,  16,  -1, -16,   0,
      1,  16,  -1, -16,  15, -15,  17, -17, 0,
     14, -14,  18, -18,  31, -31,  33, -33, 0,
      3,  -1,  12,  21,  16,   7,  12
};

static const int piece_weights[] = {
    0, 0, -100, 0, -300, -350, -500, -900,
    0, 100, 0, 0, 300, 350, 500, 900
};

static const signed char chess_knight_steps[8] = {
    14, -14, 18, -18, 31, -31, 33, -33
};

static const signed char chess_ray_steps[8] = {
    1, 16, -1, -16, 15, -15, 17, -17
};

typedef struct {
    int from;
    int to;
} ChessMove;

/*
 * Castling and en passant both depend on state that is not visible in
 * the piece placement alone, and castling moves two pieces at once.
 * Every unmake therefore restores the rights, the en passant target
 * and the one extra square a move may have touched.
 */
typedef struct {
    int piece;
    int captured;
    int rights;
    int ep;
    int extra;
} ChessUndo;

/*
 * Incremental move generation avoids fixed-size move-list buffers.
 * Every recursive search invocation owns its own iterator.
 */
typedef struct {
    int square;
    int direction;
    int target;
    int step;
    unsigned char active;
    unsigned char stage;
} ChessIterator;

char user_move[5];
char ai_move[5];

int side = CWHITE;
int depth = 2;

/*
 * Castling rights and the en passant target square are part of the
 * position. Neither can be recovered from the pieces alone.
 */
int castle_rights = CR_ALL;
int ep_square = -1;

int best_src = -1;
int best_dst = -1;

unsigned char engine_state = ENGINE_PLAY;

static unsigned char SquareOK(int square)
{
    return square >= 0 &&
           square < 128 &&
           (square & 0x88) == 0;
}

/*
 * Structural validation, not a complete test of whether a position
 * could have arisen in a legal game.
 */
static unsigned char ChessPositionOK(void)
{
    int square;
    int piece;
    int color;
    int type;
    unsigned char white_kings = 0;
    unsigned char black_kings = 0;

    for (square = 0; square < 128; ++square) {
        if (!SquareOK(square))
            continue;

        piece = board[square];

        if (piece == 0)
            continue;

        color = piece & 24;
        type = piece & 7;

        if ((piece & ~31) != 0 ||
            (color != CWHITE && color != CBLACK) ||
            type == 0)
            return 0;

        if ((type == 1 && color != CWHITE) ||
            (type == 2 && color != CBLACK))
            return 0;

        if (type == 3) {
            if (color == CWHITE)
                ++white_kings;
            else
                ++black_kings;
        }
    }

    return white_kings == 1 && black_kings == 1;
}

static int ChessFindKing(int color)
{
    int square;
    int found = -1;

    for (square = 0; square < 128; ++square) {
        if (!SquareOK(square))
            continue;

        if (board[square] == (color | 3)) {
            if (found != -1)
                return -1;

            found = square;
        }
    }

    return found;
}

/*
 * Test attacks rather than legal moves. In particular, an enemy
 * king attacks adjacent squares without recursively testing its
 * own king safety.
 */
static unsigned char ChessAttacked(int square, int attacker)
{
    int i;
    int target;
    int piece;
    int type;
    int pawn;
    int offset;
    unsigned char adjacent;

    if (!SquareOK(square) ||
        (attacker != CWHITE && attacker != CBLACK))
        return 1;

    /*
     * Work backward from the attacked square to possible pawn
     * origins. White advances toward decreasing row addresses.
     */
    pawn = attacker | (attacker == CWHITE ? 1 : 2);
    offset = attacker == CWHITE ? 16 : -16;

    target = square + offset - 1;

    if (SquareOK(target) && board[target] == pawn)
        return 1;

    target = square + offset + 1;

    if (SquareOK(target) && board[target] == pawn)
        return 1;

    for (i = 0; i < 8; ++i) {
        target = square + chess_knight_steps[i];

        if (SquareOK(target) &&
            board[target] == (attacker | 4))
            return 1;
    }

    for (i = 0; i < 8; ++i) {
        target = square + chess_ray_steps[i];
        adjacent = 1;

        while (SquareOK(target)) {
            piece = board[target];

            if (piece != 0) {
                if ((piece & 24) == attacker) {
                    type = piece & 7;

                    if ((adjacent && type == 3) ||
                        type == 7 ||
                        (i < 4 && type == 6) ||
                        (i >= 4 && type == 5))
                        return 1;
                }

                break;
            }

            adjacent = 0;
            target += chess_ray_steps[i];
        }
    }

    return 0;
}

static unsigned char ChessInCheck(int color)
{
    int king;

    if (color != CWHITE && color != CBLACK)
        return 1;

    king = ChessFindKing(color);

    /*
     * Missing or duplicate king: fail closed.
     * Never use an invalid king coordinate as an array index.
     */
    if (!SquareOK(king))
        return 1;

    return ChessAttacked(king, 24 - color);
}

/*
 * Rights are only ever withdrawn, never granted, so recomputing them
 * from the home squares after every move covers a king move, a rook
 * move, and a rook captured where it stands. ChessUnmake() restores
 * the previous value rather than trying to reconstruct it.
 */
static void ChessUpdateRights(void)
{
    if (board[SQ_E1] != (CWHITE | 3))
        castle_rights &= ~(CR_WK | CR_WQ);

    if (board[SQ_H1] != (CWHITE | 6))
        castle_rights &= ~CR_WK;

    if (board[SQ_A1] != (CWHITE | 6))
        castle_rights &= ~CR_WQ;

    if (board[SQ_E8] != (CBLACK | 3))
        castle_rights &= ~(CR_BK | CR_BQ);

    if (board[SQ_H8] != (CBLACK | 6))
        castle_rights &= ~CR_BK;

    if (board[SQ_A8] != (CBLACK | 6))
        castle_rights &= ~CR_BQ;
}

/*
 * Castling is the only move in which a king changes file by two.
 * En passant is the only pawn move that changes file without landing
 * on an occupied square. Both are therefore recognisable from the
 * move and the piece alone, which keeps ChessMove at two squares.
 */
static unsigned char ChessMoveKind(
    int piece,
    int captured,
    int from,
    int to
)
{
    int type = piece & 7;

    if ((type == 1 || type == 2) &&
        (to & 7) != (from & 7) &&
        captured == 0)
        return MOVE_EP;

    if (type == 3 && (to == from + 2 || to == from - 2))
        return MOVE_CASTLE;

    return MOVE_PLAIN;
}

/*
 * Report whether the given wing may be castled, and where the king
 * would land. Every square between king and rook must be empty, and
 * the king may not start, cross or finish on an attacked square.
 * The rook itself may be attacked, and on the queen side it may also
 * pass over an attacked square.
 */
static unsigned char ChessCastleOK(
    int color,
    unsigned char kingside,
    int *to
)
{
    int king_home;
    int rook_home;
    int enemy;
    int right;
    int step;
    int square;

    if (color == CWHITE) {
        king_home = SQ_E1;
        rook_home = kingside ? SQ_H1 : SQ_A1;
        right = kingside ? CR_WK : CR_WQ;
    } else {
        king_home = SQ_E8;
        rook_home = kingside ? SQ_H8 : SQ_A8;
        right = kingside ? CR_BK : CR_BQ;
    }

    if ((castle_rights & right) == 0)
        return 0;

    if (board[king_home] != (color | 3) ||
        board[rook_home] != (color | 6))
        return 0;

    step = kingside ? 1 : -1;

    for (square = king_home + step;
         square != rook_home;
         square += step) {
        if (board[square] != 0)
            return 0;
    }

    enemy = 24 - color;

    if (ChessAttacked(king_home, enemy) ||
        ChessAttacked(king_home + step, enemy) ||
        ChessAttacked(king_home + 2 * step, enemy))
        return 0;

    *to = king_home + 2 * step;
    return 1;
}

/*
 * Internal make/unmake operations.
 *
 * Callers must supply a generated or validated move.
 * These operations do not change side or engine_state.
 */
static void ChessMake(const ChessMove *move, ChessUndo *undo)
{
    int piece;
    int type;
    int square;

    undo->piece = board[move->from];
    undo->captured = board[move->to];
    undo->rights = castle_rights;
    undo->ep = ep_square;
    undo->extra = 0;

    piece = undo->piece;
    type = piece & 7;

    board[move->from] = 0;

    switch (ChessMoveKind(
                piece,
                undo->captured,
                move->from,
                move->to)) {
        case MOVE_EP:
            /* The captured pawn stands beside the destination. */
            square = (move->from & 0x70) | (move->to & 7);
            undo->extra = board[square];
            board[square] = 0;
            break;

        case MOVE_CASTLE:
            if (move->to > move->from) {
                undo->extra = board[move->from + 3];
                board[move->from + 3] = 0;
                board[move->from + 1] = undo->extra;
            } else {
                undo->extra = board[move->from - 4];
                board[move->from - 4] = 0;
                board[move->from - 1] = undo->extra;
            }
            break;

        default:
            break;
    }

    /* Use the same automatic promotion during search and play. */
    if ((piece == 9 && move->to < 8) ||
        (piece == 18 && move->to >= 112))
        piece |= 7;

    board[move->to] = piece;

    /* A double push exposes the square it stepped over. */
    ep_square = -1;

    if (type == 1 && move->from - move->to == 32)
        ep_square = move->to + 16;
    else if (type == 2 && move->to - move->from == 32)
        ep_square = move->to - 16;

    ChessUpdateRights();
}

static void ChessUnmake(
    const ChessMove *move,
    const ChessUndo *undo
)
{
    int square;

    board[move->from] = undo->piece;
    board[move->to] = undo->captured;

    switch (ChessMoveKind(
                undo->piece,
                undo->captured,
                move->from,
                move->to)) {
        case MOVE_EP:
            square = (move->from & 0x70) | (move->to & 7);
            board[square] = undo->extra;
            break;

        case MOVE_CASTLE:
            if (move->to > move->from) {
                board[move->from + 3] = undo->extra;
                board[move->from + 1] = 0;
            } else {
                board[move->from - 4] = undo->extra;
                board[move->from - 1] = 0;
            }
            break;

        default:
            break;
    }

    castle_rights = undo->rights;
    ep_square = undo->ep;
}

static void ChessBegin(
    ChessIterator *iterator,
    int first_square
)
{
    iterator->square = first_square;
    iterator->direction = 0;
    iterator->target = 0;
    iterator->step = 0;
    iterator->active = 0;
    iterator->stage = 0;
}

/*
 * Produce pseudo-legal moves using the original offset table.
 *
 * An iterator may be resumed only after any temporary move has
 * been unmade. Do not reuse it after committing another position.
 */
static unsigned char ChessNextPseudo(
    int color,
    ChessIterator *iterator,
    ChessMove *move
)
{
    int piece;
    int type;
    int target;
    int captured;
    unsigned char repeat;
    unsigned char pawn_forward;
    unsigned char kingside;

    while (iterator->square < 128) {
        if (!iterator->active) {
            if (!SquareOK(iterator->square) ||
                (board[iterator->square] & 24) != color) {
                ++iterator->square;
                continue;
            }

            type = board[iterator->square] & 7;

            if (type == 0) {
                ++iterator->square;
                continue;
            }

            iterator->direction = move_offsets[type + 30];
            iterator->step = 0;
            iterator->stage = 0;
            iterator->active = 1;
        }

        piece = board[iterator->square];
        type = piece & 7;

        if (iterator->step == 0) {
            /*
             * Stage zero walks the offset table. A king carries on
             * into the castling stages once its ordinary steps are
             * spent, so the table index is never advanced past the
             * terminator that ended them.
             */
            if (iterator->stage == 0) {
                ++iterator->direction;
                iterator->step = move_offsets[iterator->direction];

                if (iterator->step != 0)
                    iterator->target = iterator->square;
                else
                    iterator->stage = type == 3 ? 1 : 3;
            }

            if (iterator->step == 0) {
                while (iterator->stage < 3) {
                    kingside = iterator->stage == 1;
                    ++iterator->stage;

                    if (ChessCastleOK(color, kingside, &target)) {
                        move->from = iterator->square;
                        move->to = target;
                        return 1;
                    }
                }

                iterator->active = 0;
                iterator->stage = 0;
                ++iterator->square;
                continue;
            }
        }

        target = iterator->target + iterator->step;

        if (!SquareOK(target)) {
            iterator->step = 0;
            continue;
        }

        captured = board[target];

        if ((captured & 24) == color) {
            iterator->step = 0;
            continue;
        }

        pawn_forward =
            iterator->step == 16 || iterator->step == -16;

        if (type < 3) {
            if (pawn_forward) {
                if (captured != 0) {
                    iterator->step = 0;
                    continue;
                }
            } else if (captured == 0) {
                /*
                 * A diagonal step onto an empty square is legal only
                 * as en passant, onto the square a double push just
                 * stepped over. The rank guard keeps a stale target
                 * from ever being usable by the wrong side.
                 */
                if (target != ep_square ||
                    (target & 0x70) !=
                        (color == CWHITE ? 0x20 : 0x50)) {
                    iterator->step = 0;
                    continue;
                }
            }
        }

        iterator->target = target;

        /* Bishops, rooks and queens continue through empty squares. */
        repeat = type >= 5 && captured == 0;

        if (type < 3 && pawn_forward && captured == 0) {
            /*
             * The first forward step from the starting row permits
             * one additional step. This already checked that the
             * intermediate square is empty.
             */
            repeat =
                target - iterator->square == iterator->step &&
                (iterator->square & 0x70) ==
                    (color == CWHITE ? 0x60 : 0x10);
        }

        if (!repeat)
            iterator->step = 0;

        /*
         * Kings are not captured. A game ends when the side to move
         * has no legal move, with check distinguishing mate from
         * stalemate.
         */
        if ((captured & 7) == 3) {
            iterator->step = 0;
            continue;
        }

        move->from = iterator->square;
        move->to = target;
        return 1;
    }

    return 0;
}

static unsigned char ChessNextLegal(
    int color,
    ChessIterator *iterator,
    ChessMove *move
)
{
    ChessUndo undo;
    unsigned char safe;

    while (ChessNextPseudo(color, iterator, move)) {
        ChessMake(move, &undo);
        safe = !ChessInCheck(color);
        ChessUnmake(move, &undo);

        if (safe)
            return 1;
    }

    return 0;
}

static unsigned char ChessLegal(
    int color,
    int from,
    int to
)
{
    ChessIterator iterator;
    ChessMove move;

    if ((color != CWHITE && color != CBLACK) ||
        !SquareOK(from) ||
        !SquareOK(to) ||
        from == to ||
        !ChessPositionOK())
        return 0;

    if ((board[from] & 24) != color)
        return 0;

    ChessBegin(&iterator, from);

    while (ChessNextLegal(color, &iterator, &move)) {
        if (move.from != from)
            break;

        if (move.to == to)
            return 1;
    }

    return 0;
}

static unsigned char ChessStatus(int color)
{
    ChessIterator iterator;
    ChessMove move;

    if ((color != CWHITE && color != CBLACK) ||
        !ChessPositionOK())
        return ENGINE_INVALID;

    ChessBegin(&iterator, 0);

    if (ChessNextLegal(color, &iterator, &move))
        return ENGINE_PLAY;

    return ChessInCheck(color) ?
        ENGINE_MATE : ENGINE_STALEMATE;
}

static int ChessEvaluate(int color)
{
    int square;
    int piece;
    long total = 0;

    for (square = 0; square < 128; ++square) {
        if (!SquareOK(square))
            continue;

        piece = board[square];

        if (piece == 0)
            continue;

        total += piece_weights[piece & 15];

        if ((piece & 24) == CWHITE)
            total += starting_board[square + 8];
        else
            total -= starting_board[square + 8];
    }

    /*
     * Keep ordinary evaluations below mate scores and safely within
     * a signed 16-bit int, including unusual test positions.
     */
    if (total > 20000L)
        total = 20000L;
    else if (total < -20000L)
        total = -20000L;

    return color == CWHITE ? (int)total : -(int)total;
}

static int ChessSearch(
    int color,
    int remaining,
    int alpha,
    int beta,
    int ply
)
{
    ChessIterator iterator;
    ChessMove move;
    ChessUndo undo;
    int value;
    int best = -CHESS_INFINITY;
    unsigned char found = 0;

    ChessBegin(&iterator, 0);

    while (ChessNextLegal(color, &iterator, &move)) {
        found = 1;

        /*
         * At the horizon, first establish that at least one legal
         * move exists. Otherwise mate/stalemate would be mistaken
         * for a normal material evaluation.
         */
        if (remaining == 0)
            return ChessEvaluate(color);

        ChessMake(&move, &undo);

        value = -ChessSearch(
            24 - color,
            remaining - 1,
            -beta,
            -alpha,
            ply + 1
        );

        ChessUnmake(&move, &undo);

        if (value > best)
            best = value;

        if (value > alpha)
            alpha = value;

        if (alpha >= beta)
            break;
    }

    if (!found) {
        if (ChessInCheck(color))
            return -CHESS_MATE_SCORE + ply;

        return 0;
    }

    return best;
}

/*
 * Only root search writes best_src and best_dst.
 * Recursive searches cannot overwrite the selected root move.
 *
 * The application's caller supplies the full search window:
 *   -CHESS_INFINITY, CHESS_INFINITY.
 */
int SearchPosition(
    int color,
    int search_depth,
    int alpha,
    int beta
)
{
    ChessIterator iterator;
    ChessMove move;
    ChessUndo undo;
    int value;
    int best = -CHESS_INFINITY;

    best_src = -1;
    best_dst = -1;

    if ((color != CWHITE && color != CBLACK) ||
        !ChessPositionOK())
        return 0;

    if (search_depth < 1)
        search_depth = 1;

    if (search_depth > CHESS_MAX_DEPTH)
        search_depth = CHESS_MAX_DEPTH;

    ChessBegin(&iterator, 0);

    while (ChessNextLegal(color, &iterator, &move)) {
        ChessMake(&move, &undo);

        value = -ChessSearch(
            24 - color,
            search_depth - 1,
            -beta,
            -alpha,
            1
        );

        ChessUnmake(&move, &undo);

        /*
         * Keep a legal move even when every available move loses.
         * A losing evaluation is not permission to use stale
         * coordinates or to commit an illegal move.
         */
        if (best_src < 0 || value > best) {
            best = value;
            best_src = move.from;
            best_dst = move.to;
        }

        if (value > alpha)
            alpha = value;

        if (alpha >= beta)
            break;
    }

    if (best_src < 0) {
        if (ChessInCheck(color))
            return -CHESS_MATE_SCORE;

        return 0;
    }

    return best;
}

unsigned char xlateCol(unsigned char character)
{
    if (character < 'a' || character > 'h')
        return 255;

    return character - 'a';
}

unsigned char xlateRow(unsigned char character)
{
    if (character < '1' || character > '8')
        return 255;

    return '8' - character;
}

/*
 * The caller supplies a five-byte buffer:
 * four coordinate characters followed by a zero terminator.
 */
static unsigned char ChessParse(const char *text, ChessMove *move)
{
    unsigned char row1;
    unsigned char col1;
    unsigned char row2;
    unsigned char col2;

    if (text == 0 || move == 0)
        return 0;

    col1 = xlateCol(text[0]);
    row1 = xlateRow(text[1]);
    col2 = xlateCol(text[2]);
    row2 = xlateRow(text[3]);

    if (row1 >= 8 || col1 >= 8 ||
        row2 >= 8 || col2 >= 8 ||
        text[4] != '\0')
        return 0;

    move->from = (int)row1 * 16 + col1;
    move->to = (int)row2 * 16 + col2;

    return 1;
}

static void ChessFormatMove(
    const ChessMove *move,
    char *text
)
{
    if (!SquareOK(move->from) || !SquareOK(move->to)) {
        text[0] = '\0';
        return;
    }

    text[0] = 'a' + (move->from & 7);
    text[1] = '8' - (move->from >> 4);
    text[2] = 'a' + (move->to & 7);
    text[3] = '8' - (move->to >> 4);
    text[4] = '\0';
}

/*
 * Commit only after full validation.
 *
 * Unlike ChessMake(), this operation changes the side to move and
 * records the resulting game state.
 */
static unsigned char ChessCommit(const ChessMove *move)
{
    ChessUndo undo;

    if ((side != CWHITE && side != CBLACK) ||
        !ChessPositionOK()) {
        engine_state = ENGINE_INVALID;
        return 0;
    }

    if (!ChessLegal(side, move->from, move->to))
        return 0;

    ChessMake(move, &undo);

    side = 24 - side;
    engine_state = ChessStatus(side);

    return 1;
}

void engine_init(void)
{
    memcpy(board, starting_board, sizeof(board));

    memset(user_move, 0, sizeof(user_move));
    memset(ai_move, 0, sizeof(ai_move));

    side = CWHITE;
    depth = 2;

    castle_rights = CR_ALL;
    ep_square = -1;
    ChessUpdateRights();

    best_src = -1;
    best_dst = -1;

    engine_state = ENGINE_PLAY;
}

/* Return 1 only when a human move has actually been committed. */
unsigned char playerMove(void)
{
    ChessMove move;

    if (side != CWHITE || engine_state != ENGINE_PLAY)
        return 0;

    if (!ChessParse(user_move, &move))
        return 0;

    return ChessCommit(&move);
}

/*
 * Return 1 when an AI move was committed, even if it ended the game.
 * Return 0 when no move was committed.
 *
 * engine_state reports the resulting game state independently.
 */
unsigned char aiMove(void)
{
    ChessMove move;

    ai_move[0] = '\0';
    best_src = -1;
    best_dst = -1;

    if (side != CBLACK) {
        engine_state = ENGINE_INVALID;
        return 0;
    }

    engine_state = ChessStatus(side);

    if (engine_state != ENGINE_PLAY)
        return 0;

    SearchPosition(
        side,
        depth,
        -CHESS_INFINITY,
        CHESS_INFINITY
    );

    move.from = best_src;
    move.to = best_dst;

    if (!ChessCommit(&move)) {
        engine_state = ENGINE_INVALID;
        return 0;
    }

    ChessFormatMove(&move, ai_move);
    return 1;
}

#endif