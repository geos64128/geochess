//===================================================================================
//
//                                  GEOCHESS
//
// geoChess is a chess game for GEOS under the Commodore 64 and 128 computers
//
// Written by Scott Hutter
// Nov 2023
// 
// You are free to modify this code as desired, as long as original author credit
// is mentioned for both the geos code and the included AI engines
//===================================================================================

#ifndef GEOCHESS_H
#define GEOCHESS_H

#define VERSION 1.3
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ISGEOS64           (osType & GEOS64) == GEOS64
#define ISGEOS128          (osType & GEOS128) == GEOS128
#define C128_40_COL_MODE   (graphMode & 0x80) == 0x00
#define C128_80_COL_MODE   (graphMode & 0x80) == 0x80
#define TEMP_HIDE_MOUSE    asm("jsr $c2d7");

#define FONTBUFFERSIZE  5048 //4104
#define BOARD_TOP       33
#define BOARD_LEFT      26
#define SQUARE_WIDTH    18
#define SQUARE_HEIGHT   16

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

char osType = 0;
unsigned char sc_width = 1;
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

// last byte (64) tells soft-sprite on VDC
// if the sprite is wider than 9 pixel (bit 7 off)
// and the height of the sprite (other 7 bits)
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
 0b00000000,0b00000000,0b00000000,
 0b00010000 };

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
 0b00000000,0b00000000,0b00000000,
 0b00010000};


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


struct window vboard[8][8];         // the visual board rectangles
unsigned char gboard[8][8][2];      // the actual game board (3rd dimension is square color, piece)
char user_move[5];   
char fontbuffer[FONTBUFFERSIZE];
unsigned char sel_row1 = 255;
unsigned char sel_col1 = 255;
unsigned char tctr = 0;
unsigned char notation_row_count = 0;
unsigned char notation_text_position = 55;
enum GameStates gameState = INPROGRESS;

void_func old_otherPressVec;


// Function prototypes
void Switch4080MenuHandler(void);
void NewGameMenuHandler(void);

void InitScreen(void);
void InitBoard(unsigned char initialPosition);
void NewGame(void);

void LoadFont(void);
void hook_into_system(void);
void remove_hook(void);
void InitMovePanel(void);

void UpdateStatus(char *message);
unsigned char GetPieceChar(unsigned char row, unsigned char col);

// main menu definition

const void subMenu64 = {
	(char)12, (char)40,
	(int)0, (int)66,
	(char)(2 | VERTICAL),
	"new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
	"quit", (char)MENU_ACTION, (int)EnterDeskTop,
};

const void subMenu128_40 = {
	(char)12, (char)54,
	(int)0, (int)66,
	(char)(3 | VERTICAL),
	"new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
	"switch 40/80", (char)MENU_ACTION, (int)Switch4080MenuHandler,
	"quit", (char)MENU_ACTION, (int)EnterDeskTop,
};

const void subMenu128_80 = {
	(char)12, (char)54,
	(int)0, (int)90,
	(char)(3 | VERTICAL),
	"new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
	"switch 40/80", (char)MENU_ACTION, (int)Switch4080MenuHandler,
	"quit", (char)MENU_ACTION, (int)EnterDeskTop,
};


const void mainMenu = {
	(char)0, (char)15,
	(int)0, (int)27,
	(char)(1 | HORIZONTAL),
	"geos", (char)SUB_MENU, (int)&subMenu64,
};

#endif