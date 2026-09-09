# geoChess

Chess game for GEOS 64 / 128

![Alt text](https://i.imgur.com/2dFdKmp.png "Screenshot")

I started this game as I was rather surprised that I couldn't find any chess games for GEOS. If anyone
knows of any others, let me know.  This is a work in progress, so expect some bugs.  But as of this
initial release it is playable. 

Castling, en passant and pawn promotion are implemented. A file menu
saves and loads games to the same disk, and a bell sounds when the
engine hands the move back to you.

Known issues:
* Promotion is always to a queen; under-promotion is not offered.
* Draws by repetition, the fifty move rule and insufficient material
  are not detected.
* Selecting a piece and moving it takes more clicks than it should on
  some input drivers.

Future Additions:
 * Possible network play via freechess.org or direct
 * Switch sides
 * Undo last move
 * Under-promotion

Engine notes:
This engine was a small footprint engine from Maksim Korzh (https://www.chessprogramming.org/BMCP).
It seems to run well, and at a reasonable speed for an 8-bit machine. I tried to keep the engine decoupled
from the user interface, only passing move information.  This would allow for other engines to be added.

Please send screenshots of errant moves and I'll work on trying to correct any issues.

Testing:
The engine is plain C with no GEOS dependencies, so it can be exercised
on the desktop. `test/run.sh` (or `test/run.bat`) builds and runs a
suite with an ordinary host compiler - cc65 is not needed. It checks
move generation against the published perft counts, the castling and
en passant rules position by position, the saved game format, and
invariants over several thousand randomly played games. See
test/README.md.
