# geoChess

Chess game for GEOS 64 / 128

![Alt text](https://i.imgur.com/2dFdKmp.png "Screenshot")

I started this game as I was rather surprised that I couldnt find any chess games for GEOS. If anyone
knows of any others, let me know.  This is a work in progress, so expect some bugs.  But as of this
initial release it is playable. 

11/12/2023 - Initial beta release.  

Known issues:
* 128 80-col mode not yet added.
* Pawn promotion needs to be finalized.
* En passant rule is not implemented
* Castling is not yet implemented

Future Additions:
 * Possible network play via freechess.org or direct
 * Switch sides
 * Undo last move
 * Load/Save game

Engine notes:
This engine was a small footprint engine from Maksim Korzh (https://www.chessprogramming.org/BMCP).
It seems to run well, and at a reasonable speed for an 8-bit machine. I tried to keep the engine decoupled
from the user interface, only passing move information.  This would allow for other engines to be added.

Please send screenshots of errant moves and I'll work on trying to correct any issues.
scott _dot_ hutter _atsign_ gmail _dot_ com