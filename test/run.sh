#!/bin/sh
#
# Build and run the host-side test suite.
#
# These compile with an ordinary host compiler against src/geochess-ai.h.
# cc65 is not needed and nothing here touches GEOS.

set -e

cd "$(dirname "$0")"

CC=${CC:-gcc}
SRC=../src
BUILD=build

mkdir -p "$BUILD"

# savetest is generated from the sources so it cannot drift from them
PYTHON=${PYTHON:-python3}
if ! command -v "$PYTHON" >/dev/null 2>&1; then
    PYTHON=python
fi
"$PYTHON" mksavetest.py "$BUILD/savetest.c"

TESTS="perft rules clickpath search selfplay"
GENERATED="savetest"

fails=0

for t in $TESTS; do
    "$CC" -O2 -I"$SRC" -o "$BUILD/$t" "$t.c"
done

for t in $GENERATED; do
    "$CC" -O2 -I"$SRC" -o "$BUILD/$t" "$BUILD/$t.c"
done

for t in $TESTS $GENERATED; do
    printf '===== %s =====\n' "$t"
    if "$BUILD/$t"; then
        echo "----- $t PASSED"
    else
        echo "----- $t FAILED"
        fails=$((fails + 1))
    fi
    echo
done

if [ "$fails" -eq 0 ]; then
    echo "all suites passed"
else
    echo "$fails suite(s) failed"
    exit 1
fi
