/*
 * GEOCHESS
 *
 * Chess for GEOS on the Commodore 64 and 128.
 *
 * Original application:
 *   Scott Hutter, November 2023.
 *
 * You are free to modify this code as desired, as long as original
 * author credit is mentioned for both the GEOS code and the
 * included AI engines.
 *
 * Engine attribution and license are retained in geochess-ai.h.
 *
 * Corrected source revision: GEOCHESS_FIXSET_1.
 *
 * The engine board is authoritative. gboard is only a display cache.
 *
 * Move generation is verified host-side by perft, including castling
 * and en passant. This revision has not been hardware-tested.
 */

#include <geos.h>
#include <stdlib.h>
#include <string.h>

#include "geochess.h"
#include "geochess-ai.h"

/*
 * The generated geochess-res.h is intentionally not included.
 * The active menu resources are defined in geochess.h.
 */

static struct menu *current_menu = 0;

static char saved_status[32] = "";

static unsigned char hook_installed = 0;
static unsigned char view_ready = 0;
static unsigned char input_busy = 0;

static void ConfigureMenus(void)
{
    if (ISGEOS128) {
        if (C128_80_COL_MODE) {
            sc_width = 2;
            current_menu = (struct menu *)&mainMenu128_80;
        } else {
            sc_width = 1;
            current_menu = (struct menu *)&mainMenu128_40;
        }
    } else {
        sc_width = 1;
        current_menu = (struct menu *)&mainMenu64;
    }
}

static void HideVDCMouse(void)
{
    if (ISGEOS128) {
        if (C128_80_COL_MODE) {
            TEMP_HIDE_MOUSE;
        }
    }
}

static void DrawSquare(unsigned char row, unsigned char col);

static void ChainMouseHandler(void)
{
    if (old_otherPressVec != 0 &&
        old_otherPressVec != MouseClickHandler)
        old_otherPressVec();
}

/*
 * Clear the selection, repainting the square that carried the marker.
 *
 * The marker is drawn into the bitmap rather than floating above it in
 * a sprite, so it has to be painted out again. Repainting the whole
 * square from the display cache also restores the piece standing on
 * it, and leaves gdrawn agreeing with what is on screen.
 */
static void CancelSelection(void)
{
    unsigned char row = sel_row1;
    unsigned char col = sel_col1;

    sel_row1 = 255;
    sel_col1 = 255;
    tctr = 0;

    if (view_ready && row < 8 && col < 8) {
        HideVDCMouse();
        LoadCharSet((struct fontdesc *)fontbuffer);
        DrawSquare(row, col);
    }
}

static void SyncBoardFromEngine(void)
{
    static const unsigned char display_types[8] = {
        EMPTY,
        WHT_PAWN,
        WHT_PAWN,
        WHT_KING,
        WHT_KNIGHT,
        WHT_BISHOP,
        WHT_ROOK,
        WHT_QUEEN
    };

    unsigned char row;
    unsigned char col;
    unsigned char display_piece;
    int piece;
    int color;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col) {
            piece = board[(int)row * 16 + col];
            color = piece & 24;
            display_piece = EMPTY;

            if (piece != 0 &&
                (color == CWHITE || color == CBLACK)) {
                display_piece = display_types[piece & 7];

                if (display_piece != EMPTY && color == CBLACK)
                    display_piece += 6;
            }

            gboard[row][col] = display_piece;
        }
    }
}

static void DrawSavedStatus(void)
{
    struct window rect;

    rect.top = 181;
    rect.bot = 199;
    rect.left = 208 * sc_width;
    rect.right = sc_width == 1 ? 319 : 639;

    DrawStdRect(0, &rect);

    UseSystemFont();
    PutString(saved_status, 188, 215 * sc_width);
}

static void RedrawNotation(void)
{
    struct window rect;
    unsigned char i;
    unsigned char y;

    rect.top = 47;
    rect.bot = 175;
    rect.left = 208;
    rect.right = 310;

    DrawRect(0, &rect);

    VerticalLine(255, 45, 174, 257 * sc_width);

    UseSystemFont();
    y = 55;

    for (i = 0; i < notation_row_count; ++i) {
        if (notation_players[i] == 0)
            PutString("White", y, 215 * sc_width);
        else
            PutString("Black", y, 215 * sc_width);

        PutString(notation_moves[i], y, 275 * sc_width);
        y += 11;
    }
}

static void RefreshGameStatus(void)
{
    gameState =
        engine_state == ENGINE_PLAY ? INPROGRESS : STOPPED;

    switch (engine_state) {
        case ENGINE_PLAY:
            if (side == CBLACK)
                UpdateStatus("Thinking...");
            else if (ChessInCheck(CWHITE))
                UpdateStatus("**Check!**");
            else
                UpdateStatus("Your move.");
            break;

        case ENGINE_MATE:
            if (side == CWHITE)
                UpdateStatus("White is mated.");
            else
                UpdateStatus("Black is mated.");
            break;

        case ENGINE_STALEMATE:
            UpdateStatus("Stalemate.");
            break;

        default:
            UpdateStatus("Position error.");
            break;
    }
}

void main(void)
{
    char msg[32] =
        "GeoChess v" TOSTRING(VERSION) " fix1";

    osType = get_ostype();
    gameState = STOPPED;

    ConfigureMenus();

    if (ISGEOS128) {
        if (C128_80_COL_MODE)
            DlgBoxOk(msg, "Commodore 128 80 column mode");
        else
            DlgBoxOk(msg, "Commodore 128 40 column mode");
    } else {
        DlgBoxOk(msg, "For the Commodore 64");
    }

    if (!LoadFont()) {
        QuitGame();
        return;
    }

    InitScreen();
    view_ready = 1;

    NewGame();

    /*
     * Install the input callback only after fonts, rectangles and
     * engine state have been initialized.
     */
    hook_into_system();
    atexit(remove_hook);

    MainLoop();

    /* Also clean up if MainLoop ever returns. */
    QuitGame();
}

unsigned char LoadFont(void)
{
    char fname[15] = "geochessfont40";
    unsigned char status;

    if (ISGEOS128) {
        if (C128_80_COL_MODE)
            fname[12] = '8';
    }

    /*
     * Stop using the old font before overwriting its buffer.
     * This also keeps error dialogs on the system font.
     */
    UseSystemFont();

    status = OpenRecordFile(fname);

    if (status != 0) {
        DlgBoxOk("Error accessing fonts.", fname);
        return 0;
    }

    memset(fontbuffer, 0, sizeof(fontbuffer));

    /*
     * These checks use the cc65 GEOS zero-success convention.
     * They detect record-operation errors, not every possible
     * malformed font descriptor.
     */
    status = PointRecord(16);

    if (status == 0)
        status = ReadRecord(fontbuffer, FONTBUFFERSIZE);

    CloseRecordFile();

    if (status != 0) {
        DlgBoxOk("Cannot read font record.", fname);
        return 0;
    }

    return 1;
}

/*
 * Draw a rectangle specified in 40-column logical coordinates.
 * Unlike the original helper, this does not modify the caller's
 * rectangle in place.
 */
void DrawRect(unsigned char pattern, struct window *square)
{
    struct window scaled;

    scaled.top = square->top;
    scaled.bot = square->bot;
    scaled.left = square->left * sc_width;
    scaled.right = square->right * sc_width;

    DrawStdRect(pattern, &scaled);
}

/*
 * Build the rectangle for one square. The result is shared scratch,
 * so use it before calling this again.
 */
static struct window *SquareRect(unsigned char row, unsigned char col)
{
    sq_rect.top = sq_top[row];
    sq_rect.bot = sq_top[row] + SQUARE_HEIGHT;
    sq_rect.left = sq_left[col];
    sq_rect.right = sq_left[col] + SQUARE_WIDTH * sc_width;

    return &sq_rect;
}

/* Draw a rectangle already expressed in screen coordinates. */
void DrawStdRect(unsigned char pattern, struct window *square)
{
    SetPattern(pattern);
    InitDrawWindow(square);
    Rectangle();
}

unsigned char GetPieceChar(unsigned char row, unsigned char col)
{
    static const unsigned char glyphs[2][13] = {
        {
            EMPTY,
            WHT_KING_WHT_SQR,
            WHT_QUEEN_WHT_SQR,
            WHT_BISHOP_WHT_SQR,
            WHT_KNIGHT_WHT_SQR,
            WHT_ROOK_WHT_SQR,
            WHT_PAWN_WHT_SQR,
            BLK_KING_WHT_SQR,
            BLK_QUEEN_WHT_SQR,
            BLK_BISHOP_WHT_SQR,
            BLK_KNIGHT_WHT_SQR,
            BLK_ROOK_WHT_SQR,
            BLK_PAWN_WHT_SQR
        },
        {
            EMPTY,
            WHT_KING_BLK_SQR,
            WHT_QUEEN_BLK_SQR,
            WHT_BISHOP_BLK_SQR,
            WHT_KNIGHT_BLK_SQR,
            WHT_ROOK_BLK_SQR,
            WHT_PAWN_BLK_SQR,
            BLK_KING_BLK_SQR,
            BLK_QUEEN_BLK_SQR,
            BLK_BISHOP_BLK_SQR,
            BLK_KNIGHT_BLK_SQR,
            BLK_ROOK_BLK_SQR,
            BLK_PAWN_BLK_SQR
        }
    };

    unsigned char piece;
    unsigned char color;

    if (row >= 8 || col >= 8)
        return EMPTY;

    piece = gboard[row][col];
    color = SQUARE_COLOR(row, col);

    if (piece > BLK_PAWN || color > BLK)
        return EMPTY;

    /* EMPTY maps explicitly to zero, not an uninitialized local. */
    return glyphs[color][piece];
}

void InitScreen(void)
{
    unsigned int screen_width;
    unsigned int title_left;
    struct window rect;

    screen_width = sc_width == 1 ? 319 : 639;
    title_left = sc_width == 1 ? 200 : 400;

    HideVDCMouse();

    rect.top = 0;
    rect.bot = 20;
    rect.left = 0;
    rect.right = screen_width;
    DrawStdRect(2, &rect);

    HorizontalLine(255, 20, 0, screen_width);

    rect.top = 21;
    rect.bot = 199;
    rect.left = 0;
    rect.right = screen_width;
    DrawStdRect(0, &rect);

    rect.top = 0;
    rect.bot = 15;
    rect.left = title_left;
    rect.right = screen_width;
    DrawStdRect(0, &rect);

    HorizontalLine(255, 1, title_left, screen_width);
    HorizontalLine(255, 4, title_left, screen_width);
    HorizontalLine(255, 6, title_left, screen_width);
    HorizontalLine(255, 8, title_left, screen_width);
    HorizontalLine(255, 10, title_left, screen_width);
    HorizontalLine(255, 13, title_left, screen_width);

    UseSystemFont();
    PutString("  GeoChess  ", 9, title_left + 20);
}

void InitBoard(unsigned char initialPosition)
{
    unsigned char row;
    unsigned char col;
    unsigned char i;
    unsigned char piece;
    unsigned int x;
    unsigned int y;
    struct window rect;

    /*
     * Kept for source-interface compatibility.
     * NewGame() resets the engine; this routine only redraws it.
     */
    (void)initialPosition;

    HideVDCMouse();
    SyncBoardFromEngine();

    rect.top = BOARD_TOP;
    rect.bot = BOARD_TOP + 144;
    rect.left = BOARD_LEFT;
    rect.right = BOARD_LEFT + 160;
    DrawRect(0, &rect);

    for (i = 0; i < 9; ++i) {
        y = BOARD_TOP + i * (SQUARE_HEIGHT + 2);

        HorizontalLine(
            255,
            y,
            BOARD_LEFT * sc_width,
            (BOARD_LEFT + 160) * sc_width
        );
    }

    for (i = 0; i < 9; ++i) {
        x = (BOARD_LEFT + i * (SQUARE_WIDTH + 2)) * sc_width;

        VerticalLine(
            255,
            BOARD_TOP,
            BOARD_TOP + 144,
            x
        );
    }

    /* Rebuild the square geometry for the current mode. */
    for (row = 0; row < 8; ++row)
        sq_top[row] = BOARD_TOP + 1 + row * (SQUARE_HEIGHT + 2);

    for (col = 0; col < 8; ++col)
        sq_left[col] =
            (BOARD_LEFT + 1 + col * (SQUARE_WIDTH + 2)) * sc_width;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col) {
            DrawStdRect(
                SQUARE_COLOR(row, col),
                SquareRect(row, col)
            );
        }
    }

    UseSystemFont();

    for (col = 0; col < 8; ++col) {
        PutChar(
            'a' + col,
            184,
            (31 + 20 * col) * sc_width
        );
    }

    for (row = 0; row < 8; ++row) {
        PutChar(
            '8' - row,
            44 + 18 * row,
            10 * sc_width
        );
    }

    LoadCharSet((struct fontdesc *)fontbuffer);

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col) {
            piece = GetPieceChar(row, col);

            if (piece != EMPTY) {
                PutChar(
                    piece,
                    50 + 18 * row,
                    (27 + 20 * col) * sc_width
                );
            }

            gdrawn[row][col] = gboard[row][col];
        }
    }
}

void InitMovePanel(void)
{
    struct window rect;
    unsigned char y;

    rect.top = 44;
    rect.bot = 175;
    rect.left = 208;
    rect.right = 310;
    DrawRect(0, &rect);

    for (y = 33; y <= 45; y += 2) {
        HorizontalLine(
            255,
            y,
            207 * sc_width,
            312 * sc_width
        );
    }

    HorizontalLine(
        255, 177, 207 * sc_width, 312 * sc_width
    );

    HorizontalLine(
        255, 178, 209 * sc_width, 313 * sc_width
    );

    VerticalLine(255, 33, 177, 207 * sc_width);
    VerticalLine(255, 33, 177, 312 * sc_width);
    VerticalLine(255, 40, 178, 313 * sc_width);
    VerticalLine(255, 45, 174, 257 * sc_width);

    UseSystemFont();
    PutString("  move log  ", 40, 215 * sc_width);
}

void NewGame(void)
{
    if (input_busy)
        return;

    input_busy = 1;

    CancelSelection();

    notation_row_count = 0;

    memset(notation_moves, 0, sizeof(notation_moves));
    memset(notation_players, 0, sizeof(notation_players));
    memset(gboard, 0, sizeof(gboard));
    memset(gdrawn, 0, sizeof(gdrawn));

    /*
     * Reset every engine square before rebuilding the display.
     * No pieces or selection state survive from the old game.
     */
    engine_init();

    InitBoard(1);
    InitMovePanel();
    RefreshGameStatus();

    input_busy = 0;

    DoMenu(current_menu);
}

void UpdateNotation(
    unsigned char player,
    unsigned char src_row,
    unsigned char src_col,
    unsigned char dest_row,
    unsigned char dest_col
)
{
    ChessMove move;
    char text[5];
    unsigned char i;

    if (player > 1 ||
        src_row >= 8 ||
        src_col >= 8 ||
        dest_row >= 8 ||
        dest_col >= 8)
        return;

    move.from = (int)src_row * 16 + src_col;
    move.to = (int)dest_row * 16 + dest_col;

    /*
     * Every valid destination, including row zero/rank eight,
     * produces a complete four-character coordinate move.
     */
    ChessFormatMove(&move, text);

    /*
     * Scroll the oldest entry off the top instead of clearing the
     * log, so the panel always shows the most recent moves.
     */
    if (notation_row_count >= NOTATION_ROWS) {
        for (i = 1; i < NOTATION_ROWS; ++i) {
            strcpy(notation_moves[i - 1], notation_moves[i]);
            notation_players[i - 1] = notation_players[i];
        }

        notation_row_count = NOTATION_ROWS - 1;
    }

    strcpy(notation_moves[notation_row_count], text);
    notation_players[notation_row_count] = player;

    ++notation_row_count;

    RedrawNotation();
}

void UpdateStatus(const char *message)
{
    if (message == 0)
        message = "";

    if (message != saved_status) {
        strncpy(
            saved_status,
            message,
            sizeof(saved_status) - 1
        );

        saved_status[sizeof(saved_status) - 1] = '\0';
    }

    DrawSavedStatus();
}

/*
 * Repaint one square from the display cache.
 * The chess character set must already be loaded.
 */
static void DrawSquare(unsigned char row, unsigned char col)
{
    unsigned char piece;

    DrawStdRect(SQUARE_COLOR(row, col), SquareRect(row, col));

    piece = GetPieceChar(row, col);

    if (piece != EMPTY) {
        PutChar(
            piece,
            50 + 18 * row,
            (27 + 20 * col) * sc_width
        );
    }

    gdrawn[row][col] = gboard[row][col];
}

/*
 * Mark the selected square.
 *
 * This used to be a GEOS sprite, but a sprite bitmap is a fixed three
 * bytes across. That is 24 pixels, which suits a 40 column square but
 * covers barely half of the 36 pixel wide square in 80 column mode,
 * where the horizontal resolution doubles and the vertical does not.
 * Drawing the marker instead scales with the board, because the
 * rectangle it frames is the same one the square was drawn from.
 *
 * The border is inverted rather than drawn solid. Squares alternate
 * between a light and a dark fill pattern, and the font carries a
 * separate glyph for a piece on each, so a frame drawn in set pixels
 * would stand out on a light square and vanish into a dark one.
 * Inverting guarantees contrast against whatever is underneath.
 *
 * Inverting the outer rectangle and then the inner one flips the
 * border twice over the interior, which leaves the square and the
 * piece standing on it exactly as they were and marks only the edge.
 */
static void DrawSelection(unsigned char row, unsigned char col)
{
    HideVDCMouse();

    InitDrawWindow(SquareRect(row, col));
    InvertRectangle();

    /*
     * A two pixel border, matching the sprite this replaced. Rows are
     * the same height in both modes so the vertical inset is a flat
     * two, but 80 column pixels are half as wide, so the horizontal
     * inset scales to keep the border the same physical thickness.
     */
    sq_rect.top += 2;
    sq_rect.bot -= 2;
    sq_rect.left += 2 * sc_width;
    sq_rect.right -= 2 * sc_width;

    InitDrawWindow(&sq_rect);
    InvertRectangle();
}

/*
 * The engine has already committed the move.
 *
 * Repaint whichever squares differ from what is on screen rather than
 * assuming a move touches only its origin and destination: castling
 * moves a rook as well, and en passant empties a square the capturing
 * pawn never lands on.
 */
void RefreshBoardDisplay(void)
{
    unsigned char row;
    unsigned char col;

    HideVDCMouse();
    SyncBoardFromEngine();

    LoadCharSet((struct fontdesc *)fontbuffer);

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col) {
            if (gboard[row][col] != gdrawn[row][col])
                DrawSquare(row, col);
        }
    }
}

/*
 * Notification bell.
 *
 * The cc65 GEOS bindings expose sidbase but no sound entry point, so
 * the chip is driven directly. That makes this the only place the
 * application touches hardware itself: everything else goes through
 * GEOS kernal calls, which do their own bank switching.
 *
 * Under GEOS the I/O area cannot be assumed to be visible. A write to
 * $D400 with the wrong bank configuration lands in the RAM underneath
 * the SID and is silently lost. So the registers are bracketed with
 * the same sequence the cc65 GEOS VDC driver uses before it touches
 * $D600: disable interrupts, save the processor port, select the
 * configuration that exposes I/O, and put both back afterwards.
 *
 * $35 leaves BASIC and the KERNAL banked out, which is what GEOS
 * expects, and turns the I/O block on. Nothing between the bank
 * switch and its restore may call a GEOS routine, so the body is
 * plain register stores only.
 *
 * Voice one is used rather than voice three: voice three is the one
 * other software borrows as a noise source, and $D418 bit 7 can
 * disconnect it outright.
 *
 * The filter registers are cleared as well as the volume. A program
 * that ran earlier may have routed voices into the filter, and with
 * no filter mode selected in $D418 a routed voice never reaches the
 * output.
 *
 * Sustain is zero, so the note always decays to silence by itself and
 * can never leave a tone hanging. The gate is dropped before the
 * voice is reprogrammed and raised at the end, which both retriggers
 * the envelope and leaves the gate low long enough to be seen.
 *
 * GEOS 128 runs at 2MHz in 80 column mode, which clocks the SID at
 * twice the rate: pitch doubles and envelope times halve. That mode
 * gets its own constants so both land on roughly the same chime.
 */
#define BELL_VOICE         0       /* register offset of voice one */

#define BELL_PITCH_1MHZ    0x7512  /* about 1760 Hz */
#define BELL_PITCH_2MHZ    0x39AC  /* the same pitch at twice the clock */

#define BELL_DECAY_1MHZ    0x09    /* 2 ms attack, about 750 ms decay */
#define BELL_DECAY_2MHZ    0x0A    /* one step longer to match */

#define BELL_SID(reg)      (*(volatile unsigned char *) \
                             ((char *)sidbase + (reg)))

#define BELL_CTRL          BELL_SID(BELL_VOICE + 4)
#define BELL_VOLUME        BELL_SID(24)

static void PlayBell(void)
{
    unsigned char freq_lo;
    unsigned char freq_hi;
    unsigned char decay;

    if (sc_width == 2) {
        freq_lo = BELL_PITCH_2MHZ & 0xFF;
        freq_hi = BELL_PITCH_2MHZ >> 8;
        decay = BELL_DECAY_2MHZ;
    } else {
        freq_lo = BELL_PITCH_1MHZ & 0xFF;
        freq_hi = BELL_PITCH_1MHZ >> 8;
        decay = BELL_DECAY_1MHZ;
    }

    asm("php");
    asm("sei");
    asm("lda $01");
    asm("pha");
    asm("lda #$35");
    asm("sta $01");

    BELL_CTRL = 0x10;                   /* triangle, gate low */

    BELL_SID(BELL_VOICE + 0) = freq_lo;
    BELL_SID(BELL_VOICE + 1) = freq_hi;
    BELL_SID(BELL_VOICE + 5) = decay;
    BELL_SID(BELL_VOICE + 6) = 0x00;    /* no sustain, short release */

    BELL_SID(21) = 0x00;                /* filter cutoff low   */
    BELL_SID(22) = 0x00;                /* filter cutoff high  */
    BELL_SID(23) = 0x00;                /* nothing routed to the filter */

    BELL_VOLUME = 0x0F;                 /* volume up, filter modes off */

    BELL_CTRL = 0x11;                   /* gate high starts the note */

    asm("pla");
    asm("sta $01");
    asm("plp");
}

/* Leave the chip quiet for whatever runs after us. */
static void SilenceBell(void)
{
    asm("php");
    asm("sei");
    asm("lda $01");
    asm("pha");
    asm("lda #$35");
    asm("sta $01");

    BELL_CTRL = 0x10;
    BELL_VOLUME = 0x00;

    asm("pla");
    asm("sta $01");
    asm("plp");
}

static void HandleBoardClick(
    unsigned char row,
    unsigned char col
)
{
    unsigned char from_row;
    unsigned char from_col;
    unsigned char to_row;
    unsigned char to_col;
    ChessMove move;

    /*
     * Preserve the original event-suppression behavior.
     *
     * This remains a driver-integration limitation: replacing it
     * requires verifying which press/release/repeat events reach
     * otherPressVec on the target GEOS input driver.
     */
    if (tctr == 1 || tctr == 2) {
        ++tctr;

        if (tctr == 3)
            tctr = 0;

        return;
    }

    if (sel_row1 == 255) {
        if (gboard[row][col] >= WHT_KING &&
            gboard[row][col] <= WHT_PAWN) {

            DrawSelection(row, col);

            sel_row1 = row;
            sel_col1 = col;
            tctr = 1;
        } else {
            CancelSelection();
        }

        return;
    }

    /* Clicking the selected square again cancels the selection. */
    if (sel_row1 == row && sel_col1 == col) {
        CancelSelection();
        RefreshGameStatus();
        return;
    }

    from_row = sel_row1;
    from_col = sel_col1;

    move.from = (int)from_row * 16 + from_col;
    move.to = (int)row * 16 + col;

    ChessFormatMove(&move, user_move);

    input_busy = 1;

    /*
     * Validate and commit in the engine before changing the display
     * or adding an entry to the move log.
     */
    if (!playerMove()) {
        CancelSelection();

        if (engine_state == ENGINE_INVALID)
            RefreshGameStatus();
        else
            UpdateStatus("Illegal move.");

        /*
         * A persistent message replaces the CPU-speed-dependent
         * enable/disable loop previously used for error flashing.
         */
        input_busy = 0;
        return;
    }

    CancelSelection();

    RefreshBoardDisplay();
    UpdateNotation(0, from_row, from_col, row, col);

    RefreshGameStatus();

    if (engine_state == ENGINE_PLAY) {
        UpdateStatus("Thinking...");

        if (aiMove()) {
            from_col = xlateCol(ai_move[0]);
            from_row = xlateRow(ai_move[1]);
            to_col = xlateCol(ai_move[2]);
            to_row = xlateRow(ai_move[3]);

            /*
             * Display a committed AI move even when that move has
             * just ended the game.
             */
            RefreshBoardDisplay();

            UpdateNotation(
                1,
                from_row,
                from_col,
                to_row,
                to_col
            );
        }

        RefreshGameStatus();

        /*
         * Ring once the engine has finished and the move is actually
         * back with the player. A move that ended the game is not a
         * turn, so it stays silent.
         */
        if (engine_state == ENGINE_PLAY && side == CWHITE)
            PlayBell();
    }

    input_busy = 0;
}

void MouseClickHandler(void)
{
    unsigned char row;
    unsigned char col;

    if (!input_busy &&
        gameState == INPROGRESS &&
        side == CWHITE) {

        for (row = 0; row < 8; ++row) {
            for (col = 0; col < 8; ++col) {
                if (IsMseInRegion(SquareRect(row, col)) == 255) {
                    HandleBoardClick(row, col);
                    ChainMouseHandler();
                    return;
                }
            }
        }
    }

    ChainMouseHandler();
}

/*
 * Saved games
 *
 * The record in geochess.h is written to disk verbatim, so packing
 * and unpacking are the whole file format. Only the 64 playable
 * squares are stored; the engine rebuilds nothing else from them.
 */

#define SAVE_PACK(sq)    ((unsigned char)((((sq) >> 4) * 8) + ((sq) & 7)))
#define SAVE_UNPACK(i)   ((int)(((i) / 8) * 16 + ((i) % 8)))

static void PackGame(SaveGame *rec)
{
    unsigned char row;
    unsigned char col;
    unsigned char i;

    memset(rec, 0, sizeof(SaveGame));

    rec->magic0 = SAVE_MAGIC0;
    rec->magic1 = SAVE_MAGIC1;
    rec->version = SAVE_VERSION;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col)
            rec->squares[row * 8 + col] =
                (unsigned char)board[(int)row * 16 + col];
    }

    rec->mover = side == CBLACK ? BLK : WHT;
    rec->rights = (unsigned char)castle_rights;
    rec->ep = ep_square < 0 ?
        SAVE_NO_EP : SAVE_PACK(ep_square);
    rec->state = engine_state;

    rec->log_count = notation_row_count;

    for (i = 0; i < NOTATION_ROWS; ++i) {
        rec->log_players[i] = notation_players[i];
        strcpy(rec->log_moves[i], notation_moves[i]);
    }
}

/*
 * Validate the packed record before any of it reaches the engine.
 *
 * This repeats the structural rules ChessPositionOK() enforces, but
 * on the file data, so a damaged or foreign file is refused while the
 * game in progress is still intact.
 */
static unsigned char SaveDataOK(const SaveGame *rec)
{
    unsigned char i;
    unsigned char piece;
    unsigned char color;
    unsigned char type;
    unsigned char white_kings = 0;
    unsigned char black_kings = 0;

    if (rec->magic0 != SAVE_MAGIC0 ||
        rec->magic1 != SAVE_MAGIC1 ||
        rec->version != SAVE_VERSION)
        return 0;

    if (rec->mover > BLK ||
        rec->rights > CR_ALL ||
        rec->state > ENGINE_INVALID ||
        rec->log_count > NOTATION_ROWS)
        return 0;

    if (rec->ep != SAVE_NO_EP && rec->ep >= 64)
        return 0;

    for (i = 0; i < 64; ++i) {
        piece = rec->squares[i];

        if (piece == 0)
            continue;

        color = piece & 24;
        type = piece & 7;

        if ((piece & ~31) != 0 ||
            type == 0 ||
            (color != CWHITE && color != CBLACK))
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

static void UnpackGame(const SaveGame *rec)
{
    unsigned char row;
    unsigned char col;
    unsigned char i;

    /*
     * Start from the initial layout so the columns outside the board
     * keep exactly what a freshly begun game has in them. Nothing
     * reads them, but a restored game should not be a different shape
     * from one that was just started. The loop below then overwrites
     * every playable square, so no piece survives from before.
     */
    memcpy(board, starting_board, sizeof(board));

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 8; ++col)
            board[(int)row * 16 + col] = rec->squares[row * 8 + col];
    }

    side = rec->mover == BLK ? CBLACK : CWHITE;
    castle_rights = rec->rights;
    ep_square = rec->ep == SAVE_NO_EP ?
        -1 : SAVE_UNPACK(rec->ep);
    engine_state = rec->state;

    best_src = -1;
    best_dst = -1;

    notation_row_count = rec->log_count;

    for (i = 0; i < NOTATION_ROWS; ++i) {
        notation_players[i] = rec->log_players[i];
        strcpy(notation_moves[i], rec->log_moves[i]);
    }
}

/*
 * Rights recorded in the file are trusted only so far: withdraw any
 * that the restored placement cannot support, so a hand-edited file
 * cannot conjure a castling move out of an empty corner.
 *
 * SaveDataOK() has already passed by this point, so the placement is
 * known to be structurally sound and nothing here can fail.
 */
static void RestoreGame(const SaveGame *rec)
{
    UnpackGame(rec);
    ChessUpdateRights();
    engine_state = ChessStatus(side);
}

void LoadGameMenuHandler(void)
{
    if (input_busy)
        return;

    input_busy = 1;
    RecoverAllMenus();

    save_name[0] = '\0';

    if (DlgBoxFileSelect(SAVE_CLASS_PREFIX, APPL_DATA, save_name) != OPEN) {
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    if (FindFile(save_name) != 0) {
        DlgBoxOk("Cannot find that file.", save_name);
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    if (ReadFile(&dirEntryBuf.n_block,
                 (char *)&save_buf,
                 sizeof(save_buf)) != 0) {
        DlgBoxOk("Cannot read that file.", save_name);
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    /*
     * Validate the record while it is still only file data. Nothing
     * has been handed to the engine yet, so a bad file leaves the
     * game in progress exactly as it was and there is nothing to
     * undo.
     */
    if (!SaveDataOK(&save_buf)) {
        DlgBoxOk("That is not a valid", "GeoChess saved game.");
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    CancelSelection();
    RestoreGame(&save_buf);

    InitBoard(1);
    InitMovePanel();
    RedrawNotation();
    RefreshGameStatus();

    input_busy = 0;

    /*
     * Menus only open on the player's turn, so a restored game hands
     * the move straight back. Announce it the same way the engine
     * does when it finishes thinking.
     */
    if (engine_state == ENGINE_PLAY && side == CWHITE)
        PlayBell();

    DoMenu(current_menu);
}

/*
 * A plain document icon for the deskTop: three bytes across by
 * twenty-one rows, which is the 63 bytes a GEOS file header carries.
 */
static void BuildSaveIcon(struct fileheader *hdr)
{
    unsigned char row;

    hdr->icon_desc[0] = 0x80;
    hdr->icon_desc[1] = 0x03;
    hdr->icon_desc[2] = 0x15;

    for (row = 0; row < 21; ++row) {
        if (row == 0 || row == 20) {
            hdr->icon_pic[row * 3] = 0xFF;
            hdr->icon_pic[row * 3 + 1] = 0xFF;
            hdr->icon_pic[row * 3 + 2] = 0xFF;
        } else {
            hdr->icon_pic[row * 3] = 0x80;
            hdr->icon_pic[row * 3 + 1] = 0x00;
            hdr->icon_pic[row * 3 + 2] = 0x01;
        }
    }
}

void SaveGameMenuHandler(void)
{
    /*
     * The header is a local rather than another static: it is only
     * live for the SaveFile() call, and the C stack is already
     * reserved whether or not this uses it.
     */
    struct fileheader hdr;

    if (input_busy)
        return;

    input_busy = 1;
    RecoverAllMenus();

    save_name[0] = '\0';

    /*
     * The cc65 string dialog is built with a CANCEL icon and no OK
     * icon, so it never returns OK: the user confirms with RETURN.
     * Test for the give-up case instead of testing for confirmation.
     */
    if (DlgBoxGetString(save_name, SAVE_NAME_MAX,
                        "Save this game as:", "") == CANCEL) {
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    if (save_name[0] == '\0') {
        DlgBoxOk("No name was entered.", "Nothing saved.");
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    /* FindFile returns zero when the name is already taken */
    if (FindFile(save_name) == 0) {
        DlgBoxOk("A file of that name", "already exists.");
        input_busy = 0;
        DoMenu(current_menu);
        return;
    }

    PackGame(&save_buf);

    memset(&hdr, 0, sizeof(hdr));
    BuildSaveIcon(&hdr);

    /*
     * SaveFile() reuses the header's dead next-block link as a
     * pointer to the DOS filename, as the cc65 GEOS bindings
     * document. The name must stay put until the call returns, which
     * is why save_name is static.
     */
    hdr.n_block.track = (char)((unsigned)save_name & 0xFF);
    hdr.n_block.sector = (char)((unsigned)save_name >> 8);

    hdr.dostype = (char)(0x80 | USR);
    hdr.type = APPL_DATA;
    hdr.structure = SEQUENTIAL;

    hdr.load_address = (unsigned)&save_buf;
    hdr.end_address = (unsigned)&save_buf + sizeof(save_buf);
    hdr.exec_address = (unsigned)&save_buf;

    strcpy(hdr.class_name, SAVE_CLASS);
    strcpy(hdr.author, "GeoChess");

    if (SaveFile(0, &hdr) != 0)
        DlgBoxOk("Could not save the game.", save_name);
    else
        DlgBoxOk("Game saved as", save_name);

    DrawSavedStatus();

    input_busy = 0;

    DoMenu(current_menu);
}

void NewGameMenuHandler(void)
{
    if (input_busy)
        return;

    RecoverAllMenus();
    NewGame();
}

void Switch4080MenuHandler(void)
{
    if (input_busy)
        return;

    input_busy = 1;

    RecoverAllMenus();
    CancelSelection();

    if (ISGEOS128) {
        SetNewMode();

        /* Configure from the resulting mode, not a predicted mode. */
        ConfigureMenus();

        if (!LoadFont()) {
            input_busy = 0;
            QuitGame();
            return;
        }

        InitScreen();
        InitBoard(1);
        InitMovePanel();

        /*
         * Redraw the current notation page and preserve the status.
         * A mode switch must not reset the game or replace mate,
         * stalemate, check or error text with "Your move."
         */
        RedrawNotation();
        DrawSavedStatus();
    }

    input_busy = 0;

    DoMenu(current_menu);
}

void hook_into_system(void)
{
    if (!hook_installed) {
        old_otherPressVec = otherPressVec;
        otherPressVec = MouseClickHandler;
        hook_installed = 1;
    }
}

void remove_hook(void)
{
    if (hook_installed) {
        /*
         * Do not overwrite a different callback installed after ours.
         */
        if (otherPressVec == MouseClickHandler)
            otherPressVec = old_otherPressVec;

        hook_installed = 0;
    }

    /* Menu resources are static storage. Never pass them to free(). */
}

void QuitGame(void)
{
    input_busy = 1;
    gameState = STOPPED;

    /*
     * The menu's desktop exit may bypass the C exit machinery.
     * Restore our callback explicitly rather than relying only on
     * the atexit handler.
     */
    remove_hook();
    CancelSelection();
    SilenceBell();

    UseSystemFont();

    view_ready = 0;

    EnterDeskTop();
}