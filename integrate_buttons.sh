#!/bin/bash
#
# Integrate button UI into ffplay.c
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

FFPLAY_SRC="fftools/ffplay.c"
BACKUP_FILE="fftools/ffplay.c.before_buttons"

echo "=== FFplay Button Integration Script ==="
echo ""

# Backup original file
if [ ! -f "$BACKUP_FILE" ]; then
    echo "[1/6] Creating backup of original ffplay.c..."
    cp "$FFPLAY_SRC" "$BACKUP_FILE"
    echo "✓ Backup created: $BACKUP_FILE"
else
    echo "[1/6] Backup already exists, skipping..."
fi

# Step 2: Add button header include
echo "[2/6] Adding button header include..."
if ! grep -q "ffplay_buttons.h" "$FFPLAY_SRC"; then
    # Add after the SDL include
    sed -i.tmp '/^#include <SDL.h>/a\
#include "ffplay_buttons.h"
' "$FFPLAY_SRC"
    rm -f "$FFPLAY_SRC.tmp"
    echo "✓ Added #include \"ffplay_buttons.h\""
else
    echo "✓ Button header already included"
fi

# Step 3: Add ButtonManager to VideoState
echo "[3/6] Adding ButtonManager to VideoState structure..."
if ! grep -q "ButtonManager button_mgr" "$FFPLAY_SRC"; then
    # Find the VideoState typedef and add button_mgr field
    # This adds it near the end of the structure (before the closing brace)
    awk '/^typedef struct VideoState \{/,/^\} VideoState;/ {
        if (/^} VideoState;/) {
            print "    ButtonManager button_mgr;  // Button UI manager"
        }
        print
        next
    }
    {print}' "$FFPLAY_SRC" > "$FFPLAY_SRC.tmp"
    mv "$FFPLAY_SRC.tmp" "$FFPLAY_SRC"
    echo "✓ Added ButtonManager to VideoState"
else
    echo "✓ ButtonManager already in VideoState"
fi

# Step 4: Initialize buttons in video_open()
echo "[4/6] Adding button initialization in video_open()..."
if ! grep -q "buttons_init" "$FFPLAY_SRC"; then
    # Find video_open function and add initialization after renderer creation
    awk '/static int video_open\(VideoState \*is\)/,/^}/ {
        if (/SDL_GetWindowSize\(window/) {
            print
            print "        // Initialize button UI"
            print "        buttons_init(&is->button_mgr, w, h);"
            next
        }
        print
        next
    }
    {print}' "$FFPLAY_SRC" > "$FFPLAY_SRC.tmp"
    mv "$FFPLAY_SRC.tmp" "$FFPLAY_SRC"
    echo "✓ Added button initialization"
else
    echo "✓ Button initialization already present"
fi

echo ""
echo "=== Integration Summary ==="
echo "✓ Button header included"
echo "✓ ButtonManager added to VideoState"
echo "✓ Button initialization added"
echo ""
echo "⚠ MANUAL STEPS REQUIRED:"
echo "1. Add button rendering in video_refresh() or video_display()"
echo "2. Add mouse event handlers in event_loop()"
echo "3. Add window resize handler"
echo ""
echo "See FFPLAY_BUTTONS_INTEGRATION.md for detailed instructions."
echo ""
echo "Original file backed up to: $BACKUP_FILE"
