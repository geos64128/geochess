#ifndef GEOCHESS_H
#define GEOCHESS_H

#include <geos.h>
#include <stdlib.h>
#include <string.h>

#define C128_40_COL_MODE   (graphMode & 0x80) == 0x00
#define C128_80_COL_MODE   (graphMode & 0x80) == 0x80

void initScreen(void);

char osType = 0;
char vdc = 0;
unsigned short screen_pixel_width;

struct window *winChessBoard;
struct window *winHeader;
struct window *winBottomRow;

struct window vic_winHdr        = {0, 15, 200, SC_PIX_WIDTH-1 };
struct window vic_winChessBoard = {20, SC_PIX_HEIGHT-1, 0, SC_PIX_WIDTH-1};
struct window vic_BtmRow        = {184, SC_PIX_HEIGHT-1, 0, SC_PIX_WIDTH-1};

struct window vdc_winHdr        = {0, 15, 400, SCREENPIXELWIDTH-1 };
struct window vdc_winChessBoard = {20, SC_PIX_HEIGHT-1, 0, SCREENPIXELWIDTH-1};
struct window vdc_BtmRow        = {179, SC_PIX_HEIGHT-1, 0, SCREENPIXELWIDTH-1};

#define FONTBUFFERSIZE 4104
#define BOARD_TOP 33
#define BOARD_LEFT 26
#define SQUARE_WIDTH 18
#define SQUARE_HEIGHT 16

#define EMPTY       0
#define WHT_KING    1
#define WHT_QUEEN   2
#define WHT_BISHOP  3
#define WHT_KNIGHT  4
#define WHT_ROOK    5
#define WHT_PAWN    6
#define BLK_KING    7
#define BLK_QUEEN   8
#define BLK_BISHOP  9
#define BLK_KNIGHT  10
#define BLK_ROOK    11
#define BLK_PAWN    12

#define WHT   0
#define BLK   1

#define WHT_KING_WHT_SQR    'A'
#define WHT_QUEEN_WHT_SQR   'B'
#define WHT_BISHOP_WHT_SQR  'C'
#define WHT_KNIGHT_WHT_SQR  'D'
#define WHT_ROOK_WHT_SQR    'E'
#define WHT_PAWN_WHT_SQR    'F'

#define WHT_KING_BLK_SQR    'S'
#define WHT_QUEEN_BLK_SQR   'T'
#define WHT_BISHOP_BLK_SQR  'U'
#define WHT_KNIGHT_BLK_SQR  'V'
#define WHT_ROOK_BLK_SQR    'W'
#define WHT_PAWN_BLK_SQR    'X'

#define BLK_KING_WHT_SQR    'M'
#define BLK_QUEEN_WHT_SQR   'N'
#define BLK_BISHOP_WHT_SQR  'O'
#define BLK_KNIGHT_WHT_SQR  'P'
#define BLK_ROOK_WHT_SQR    'Q'
#define BLK_PAWN_WHT_SQR    'R'

#define BLK_KING_BLK_SQR    'G'
#define BLK_QUEEN_BLK_SQR   'H'
#define BLK_BISHOP_BLK_SQR  'I'
#define BLK_KNIGHT_BLK_SQR  'J'
#define BLK_ROOK_BLK_SQR    'K'
#define BLK_PAWN_BLK_SQR    'L'

const char square_cursor[] = { 
 0b11111111,0b11111111,0b11100000,
 0b11111111,0b11111111,0b11100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11000000,0b00000000,0b01100000,
 0b11111111,0b11111111,0b11100000,
 0b11111111,0b11111111,0b11100000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000 };

 const char badmove_cursor[] = { 
 0b11111111,0b11111111,0b11100000,
 0b11111111,0b11111111,0b11100000,
 0b11100000,0b00000000,0b11100000,
 0b11010000,0b00000001,0b01100000,
 0b11001000,0b00000010,0b01100000,
 0b11000100,0b00000100,0b01100000,
 0b11000010,0b00001000,0b01100000,
 0b11000001,0b11110000,0b01100000,
 0b11000001,0b11110000,0b01100000,
 0b11000010,0b00001000,0b01100000,
 0b11000100,0b00000100,0b01100000,
 0b11001000,0b00000010,0b01100000,
 0b11010000,0b00000001,0b01100000,
 0b11100000,0b00000000,0b11100000,
 0b11111111,0b11111111,0b11100000,
 0b11111111,0b11111111,0b11100000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000,
 0b00000000,0b00000000,0b00000000};

// the actual game board (3rd dimension is square color, piece)
unsigned char gboard[8][8][2];

// game board notation
char *gbnotation[8][8] = {
    {"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"},
    {"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7"},
    {"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6"},
    {"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5"},
    {"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4"},
    {"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3"},
    {"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2"},
    {"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"}
};

enum GameStates {
    INPROGRESS,
    STOPPED
};

// the visual board rectangles
struct window vboard[8][8];  
unsigned char gboard[8][8][2];
char user_move[5];   
unsigned char fontbuff40[FONTBUFFERSIZE];
unsigned char sel_row1 = 255;
unsigned char sel_col1 = 255;
unsigned char tctr = 0;
unsigned char notation_row_count = 0;
unsigned char notation_text_position = 55;
enum GameStates gameState = INPROGRESS;

void_func old_otherPressVec;

// check engine variables =========================================================================
// 
// this code call be replaced.  GeoChess just needs to be able to translate in chess notation
// between player and AI moves.
//
// ================================================================================================
int starting_board[128] = {                 // 0x88 board + positional scores

    22, 20, 21, 23, 19, 21, 20, 22,    0,  0,  5,  5,  0,  0,  5,  0, 
    18, 18, 18, 18, 18, 18, 18, 18,    5,  5,  0,  0,  0,  0,  5,  5,
     0,  0,  0,  0,  0,  0,  0,  0,    5, 10, 15, 20, 20, 15, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,    5, 10, 20, 30, 30, 20, 10,  5,    
     0,  0,  0,  0,  0,  0,  0,  0,    5, 10, 20, 30, 30, 20, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,    5, 10, 15, 20, 20, 15, 10,  5,
     9,  9,  9,  9,  9,  9,  9,  9,    5,  5,  0,  0,  0,  0,  5,  5,
    14, 12, 13, 15, 11, 13, 12, 14,    0,  0,  5,  5,  0,  0,  5,  0

};

int board[128];

char *notation[] = {           // convert square id to board notation

    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",     "i8","j8","k8","l8","m8","n8","o8", "p8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",     "i7","j7","k7","l7","m7","n7","o7", "p7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",     "i6","j6","k6","l6","m6","n6","o6", "p6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",     "i5","j5","k5","l5","m5","n5","o5", "p5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",     "i4","j4","k4","l4","m4","n4","o4", "p4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",     "i3","j3","k3","l3","m3","n3","o3", "p3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",     "i2","j2","k2","l2","m2","n2","o2", "p2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",     "i1","j1","k1","l1","m1","n1","o1", "p1",

};

//enum { WHITE = 8, BLACK = 16};    // side to move
#define CWHITE 8;
#define CBLACK 16;

static int move_offsets[] = {

   15,  16,  17,   0,
  -15, -16, -17,   0,
    1,  16,  -1, -16,   0,
    1,  16,  -1, -16,  15, -15, 17, -17,  0,
   14, -14,  18, -18,  31, -31, 33, -33,  0, 
    3,  -1,  12,  21,  16,   7, 12

};

int piece_weights[] = { 0, 0, -100, 0, -300, -350, -500, -900, 0, 100, 0, 0, 300, 350, 500, 900 };
int best_src, best_dst;    // to store the best move found in search

char user_move[5];
char ai_move[5];
int depth = 2;
int side = CWHITE;
int user_src, user_dst;

int sq = 0;
int score =0;

unsigned char ch = 0;
unsigned char x = 0;

void engine_init(void)
{
    unsigned char i;

    for (i = 0; i < 128; i++)
        board[i] = starting_board[i];

    side = CWHITE;
    depth = 2;

}

int SearchPosition(int side, int depth, int alpha, int beta)
{
    int mat_score = 0;
    int pos_score = 0;
    int pce = 0;
    int eval = 0;

    int old_alpha = alpha;
    int temp_src;
    int temp_dst;
    int score = -10000;

    int piece, type, directions, dst_square, captured_square, captured_piece, step_vector;
    int sq = 0;
    int src_square = 0;

    if(!depth)
    {
        // Evaluate position        
        //int mat_score = 0, pos_score = 0, pce, eval = 0;
    
        for(sq = 0; sq < 128; sq++)
        {
            if(!(sq & 0x88))
            {
                if(pce = board[sq])
                {
                    mat_score += piece_weights[pce & 15]; // material score
                    (pce & 8) ? (pos_score += board[sq + 8]) : (pos_score -= board[sq + 8]); // positional score
                }
            }
        }
    
        eval = mat_score + pos_score;

        return (side == 8) ? eval : -eval;   // here returns current position's score
    }



    // Generate moves

    for(src_square = 0; src_square < 128; src_square++)
    {
        if(!(src_square & 0x88))
        {
            piece = board[src_square];
                                        
            if(piece & side)
            {
                type = piece & 7;
                directions = move_offsets[type + 30];
                
                while(step_vector = move_offsets[++directions])
                {
                    dst_square = src_square;
                    
                    do
                    {
                        dst_square += step_vector;
                        captured_square = dst_square;
                        
                        if(dst_square & 0x88) break;
    
                        captured_piece = board[captured_square];                        
    
                        if(captured_piece & side) break;
                        if(type < 3 && !(step_vector & 7) != !captured_piece) break;
                        if((captured_piece & 7) == 3) return 10000;    // on king capture
                        
                        // make move
                        board[captured_square] = 0;
                        board[src_square] = 0;
                        board[dst_square] = piece;

                        // pawn promotion
                        if(type < 3)
                        {
                            if(dst_square + step_vector + 1 & 0x80)
                                board[dst_square]|=7;
                        }
                        
                        score = -SearchPosition(24 - side, depth - 1, -beta, -alpha);
                                              
                        // take back
                        board[dst_square] = 0;
                        board[src_square] = piece;
                        board[captured_square] = captured_piece;

                        //Needed to detect checkmate
                        best_src = src_square;
                        best_dst = dst_square;

                        // alpha-beta stuff
                        if(score > alpha)
                        {
                            if(score >= beta)
                                return beta;
                            
                            alpha = score;
        
                            temp_src = src_square;
                            temp_dst = dst_square;
                        }              
                        
                        captured_piece += type < 5;
                        
                        if(type < 3 & 6*side + (dst_square & 0x70) == 0x80)captured_piece--;  
                    }
    
                    while(!captured_piece);
                }
            }
        }
    }

    // store the best move
    if(alpha != old_alpha)
    {
        best_src = temp_src;
        best_dst = temp_dst;
    }

    return alpha;   // here returns the best score
}

void playerMove()
{
        // usermove must contain a chess notation value (eg.  c2c4)
        user_move[4] = 0;
        for(sq = 0; sq < 128; sq++)
        {
            if(!(sq & 0x88))
            {
                if(!strncmp(user_move, notation[sq], 2))
                    user_src = sq;

                if(!strncmp(user_move + 2, notation[sq], 2))
                    user_dst = sq;
            }
        }
        
        // make user move
        board[user_dst] = board[user_src];
        board[user_src] = 0;

        // if pawn promotion
    //    if(((board[user_dst] == 9) && (user_dst >= 0 && user_dst <= 7)) ||
    //       ((board[user_dst] == 18) && (user_dst >= 112 && user_dst <= 119)))
     //       board[user_dst] |= 7;
        
}

unsigned char aiMove()
{
    // aimove[5] - updates a global zero terminated string

    side = 24 - side;   // change side
            
    score = SearchPosition(side, depth, -10000, 10000);

    // make AI move
    board[best_dst] = board[best_src];
    board[best_src] = 0;

    // if pawn promotion
   // if(((board[best_dst] == 9) && (best_dst >= 0 && best_dst <= 7)) ||
    //    ((board[best_dst] == 18) && (best_dst >= 112 && best_dst <= 119)))
    //    board[best_dst] |= 7;

    side = 24 - side;    // change side

    // Checkmate detection
    if(score == 10000 || score == -10000) { return 1;}

    strcpy(ai_move, notation[best_src]);
    strcat(ai_move, notation[best_dst]);

    return 0;
}

unsigned char xlateCol(unsigned char x)
{
    unsigned char c;

    switch(x)
    {
        case 'a': { c=0; break; }
        case 'b': { c=1; break; }
        case 'c': { c=2; break; }
        case 'd': { c=3; break; }
        case 'e': { c=4; break; }
        case 'f': { c=5; break; }
        case 'g': { c=6; break; }
        case 'h': { c=7; break; }
    }

    return c;
}

unsigned char xlateRow(unsigned char x)
{
    unsigned char r;

    switch(x)
    {
        case '1': { r=7; break; }
        case '2': { r=6; break; }
        case '3': { r=5; break; }
        case '4': { r=4; break; }
        case '5': { r=3; break; }
        case '6': { r=2; break; }
        case '7': { r=1; break; }
        case '8': { r=0; break; }
    }

    return r;
}

#endif