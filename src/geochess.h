/*
 * GEOCHESS
 *
 * Chess for GEOS on the Commodore 64 and 128.
 *
 * Original application:
 *   Scott Hutter, November 2023.
 *
 * Original permission:
 *   You are free to modify this code as desired, as long as original
 *   author credit is mentioned for both the GEOS code and the
 *   included AI engines.
 *
 * Corrected source revision: GEOCHESS_FIXSET_1.
 *
 * This header contains application storage and cc65 GEOS menu
 * resources. Include it in geochess.c only.
 */

#ifndef GEOCHESS_H
#define GEOCHESS_H

#include <geos.h>

#define VERSION 1.3
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ISGEOS64           (((osType) & GEOS64) == GEOS64)
#define ISGEOS128          (((osType) & GEOS128) == GEOS128)

#define C128_40_COL_MODE    (((graphMode) & 0x80) == 0x00)
#define C128_80_COL_MODE    (((graphMode) & 0x80) == 0x80)

/* Retained from the original C128 drawing path. */
#define TEMP_HIDE_MOUSE    asm("jsr $c2d7")

#define FONTBUFFERSIZE     4928

#define BOARD_TOP          33
#define BOARD_LEFT         26
#define SQUARE_WIDTH       18
#define SQUARE_HEIGHT      16

#define NOTATION_ROWS      11

#define EMPTY              0

#define WHT_KING           1
#define WHT_QUEEN          2
#define WHT_BISHOP         3
#define WHT_KNIGHT         4
#define WHT_ROOK           5
#define WHT_PAWN           6

#define BLK_KING           7
#define BLK_QUEEN          8
#define BLK_BISHOP         9
#define BLK_KNIGHT         10
#define BLK_ROOK           11
#define BLK_PAWN           12

#define WHT                0
#define BLK                1

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

enum GameStates {
    INPROGRESS,
    STOPPED
};

/*
 * Saved game record.
 *
 * Written to disk verbatim, so the layout is the file format. The
 * board is packed to one byte per playable square: the engine keeps
 * an int per 0x88 entry, but no piece code exceeds 23 and the
 * off-board columns hold no game state.
 *
 * A magic and a version go first so a file from another program, or
 * from a later revision, is rejected before anything is disturbed.
 */
#define SAVE_MAGIC0        'G'
#define SAVE_MAGIC1        'C'
#define SAVE_VERSION       1

#define SAVE_NO_EP         255

#define SAVE_CLASS         "GeoChess Game"
#define SAVE_CLASS_PREFIX  "GeoChess"

#define SAVE_NAME_MAX      16

typedef struct {
    unsigned char magic0;
    unsigned char magic1;
    unsigned char version;
    unsigned char squares[64];
    unsigned char mover;             /* WHT or BLK */
    unsigned char rights;            /* castling rights bits */
    unsigned char ep;                /* packed square, or SAVE_NO_EP */
    unsigned char state;             /* engine_state */
    unsigned char log_count;
    unsigned char log_players[NOTATION_ROWS];
    char log_moves[NOTATION_ROWS][5];
    unsigned char reserved[2];
} SaveGame;

static unsigned char osType = 0;
static unsigned char sc_width = 1;

/*
 * Square geometry, already scaled for the current mode.
 *
 * Top and bottom depend only on the rank and left and right only on
 * the file, so two small tables and one scratch rectangle replace the
 * sixty-four windows this used to keep.
 */
static unsigned char sq_top[8];
static unsigned int sq_left[8];
static struct window sq_rect;

/*
 * Rendering cache: the display piece code per square.
 *
 * Square colour is not stored. It is (row + column) & 1 and never
 * changes, so computing it costs less than keeping a second plane.
 *
 * The engine's board[] is the authoritative chess position.
 */
static unsigned char gboard[8][8];

#define SQUARE_COLOR(row, col)   (((row) + (col)) & 1)

/*
 * What is currently on screen. Comparing it against gboard finds the
 * squares a move actually changed, which is two for an ordinary move,
 * three for en passant and four for castling.
 */
static unsigned char gdrawn[8][8];

static char fontbuffer[FONTBUFFERSIZE];

/*
 * save_buf holds the record being written or the one just read. It is
 * fully validated before any of it reaches the engine, so a bad file
 * never disturbs the game in progress and nothing has to be rolled
 * back. The 256 byte file header is a local in the save handler
 * rather than another static.
 */
static SaveGame save_buf;
static char save_name[SAVE_NAME_MAX + 1];

static unsigned char sel_row1 = 255;
static unsigned char sel_col1 = 255;
static unsigned char tctr = 0;

static unsigned char notation_row_count = 0;
static char notation_moves[NOTATION_ROWS][5];
static unsigned char notation_players[NOTATION_ROWS];

static enum GameStates gameState = STOPPED;
static void_func old_otherPressVec = 0;

/* Application entry points and menu callbacks. */
void NewGameMenuHandler(void);
void LoadGameMenuHandler(void);
void SaveGameMenuHandler(void);
void Switch4080MenuHandler(void);
void QuitGame(void);
void MouseClickHandler(void);

void InitScreen(void);
void InitBoard(unsigned char initialPosition);
void InitMovePanel(void);
void NewGame(void);

unsigned char LoadFont(void);

void DrawRect(unsigned char pattern, struct window *square);
void DrawStdRect(unsigned char pattern, struct window *square);

unsigned char GetPieceChar(unsigned char row, unsigned char col);

void RefreshBoardDisplay(void);

void UpdateNotation(
    unsigned char player,
    unsigned char src_row,
    unsigned char src_col,
    unsigned char dest_row,
    unsigned char dest_col
);

void UpdateStatus(const char *message);

void hook_into_system(void);
void remove_hook(void);

/*
 * cc65 GEOS resource declarations.
 *
 * Separate main-menu resources avoid modifying const menu storage
 * through a cast when switching display modes.
 *
 * These are the active menus. Do not also include geochess-res.h.
 */

const void subMenu64 = {
    (char)12, (char)40,
    (int)0, (int)66,
    (char)(2 | VERTICAL),
    "new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
    "quit", (char)MENU_ACTION, (int)QuitGame,
};

const void subMenu128_40 = {
    (char)12, (char)54,
    (int)0, (int)66,
    (char)(3 | VERTICAL),
    "new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
    "switch 40/80", (char)MENU_ACTION, (int)Switch4080MenuHandler,
    "quit", (char)MENU_ACTION, (int)QuitGame,
};

const void subMenu128_80 = {
    (char)12, (char)54,
    (int)0, (int)90,
    (char)(3 | VERTICAL),
    "new game", (char)MENU_ACTION, (int)NewGameMenuHandler,
    "switch 40/80", (char)MENU_ACTION, (int)Switch4080MenuHandler,
    "quit", (char)MENU_ACTION, (int)QuitGame,
};

/*
 * The file sub-menus drop from under the second title, so their left
 * edge starts where the "geos" title ends.
 */
/* Both 40 column modes share one file menu. */
const void fileMenu40 = {
    (char)12, (char)40,
    (int)27, (int)107,
    (char)(2 | VERTICAL),
    "load game", (char)MENU_ACTION, (int)LoadGameMenuHandler,
    "save game", (char)MENU_ACTION, (int)SaveGameMenuHandler,
};

const void fileMenu128_80 = {
    (char)12, (char)40,
    (int)35, (int)145,
    (char)(2 | VERTICAL),
    "load game", (char)MENU_ACTION, (int)LoadGameMenuHandler,
    "save game", (char)MENU_ACTION, (int)SaveGameMenuHandler,
};

/*
 * Two titles now, so the bar is widened to cover both. A horizontal
 * menu lays its titles out from the left edge using the system font,
 * and over-reserving only widens the strip that responds to a click.
 */
const void mainMenu64 = {
    (char)0, (char)15,
    (int)0, (int)58,
    (char)(2 | HORIZONTAL),
    "geos", (char)SUB_MENU, (int)&subMenu64,
    "file", (char)SUB_MENU, (int)&fileMenu40,
};

const void mainMenu128_40 = {
    (char)0, (char)15,
    (int)0, (int)58,
    (char)(2 | HORIZONTAL),
    "geos", (char)SUB_MENU, (int)&subMenu128_40,
    "file", (char)SUB_MENU, (int)&fileMenu40,
};

const void mainMenu128_80 = {
    (char)0, (char)15,
    (int)0, (int)74,
    (char)(2 | HORIZONTAL),
    "geos", (char)SUB_MENU, (int)&subMenu128_80,
    "file", (char)SUB_MENU, (int)&fileMenu128_80,
};

#endif