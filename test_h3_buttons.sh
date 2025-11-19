#!/bin/bash
#
# Test script for H3-enabled ffplay with button UI
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FFPLAY="$SCRIPT_DIR/ffplay"

echo "========================================="
echo "FFplay H3 + Buttons Test"
echo "========================================="
echo ""

# Check binary
if [ ! -f "$FFPLAY" ]; then
    echo "❌ Error: ffplay not found at $FFPLAY"
    exit 1
fi
echo "✓ FFplay binary found: $FFPLAY"

# Check size
SIZE=$(ls -lh "$FFPLAY" | awk '{print $5}')
echo "✓ Binary size: $SIZE"

# Check H3 protocol support
echo ""
echo "--- Protocol Support ---"
if "$FFPLAY" -protocols 2>&1 | grep -q "^ *h3$"; then
    echo "✓ H3 protocol: ENABLED"
else
    echo "❌ H3 protocol: NOT FOUND"
    exit 1
fi

# Check for button symbols
echo ""
echo "--- Button Integration Check ---"
if strings "$FFPLAY" | grep -qi "button"; then
    echo "✓ Button code found in binary"
    BUTTON_COUNT=$(strings "$FFPLAY" | grep -i button | wc -l)
    echo "  Found $BUTTON_COUNT button-related strings"
else
    echo "⚠️  Warning: No button strings found"
fi

# Show configuration
echo ""
echo "--- Build Configuration ---"
"$FFPLAY" -version 2>&1 | grep "configuration:" | sed 's/configuration: //'

echo ""
echo "========================================="
echo "Manual Test Instructions:"
echo "========================================="
echo ""
echo "1. Test H3 Playback:"
echo "   ./ffplay h3://120.79.21.28:8443/600k_cbr.flv"
echo ""
echo "2. What to verify:"
echo "   ✓ Video plays smoothly"
echo "   ✓ Button bar appears at bottom of window"
echo "   ✓ Buttons visible: Play/Pause, Stop, Seek Back, Seek Forward"
echo "   ✓ Mouse hover changes button color"
echo "   ✓ Clicking Play/Pause toggles playback"
echo "   ✓ Clicking Stop exits player"
echo "   ✓ Clicking Seek buttons jumps 10 seconds"
echo ""
echo "3. Keyboard controls (standard ffplay):"
echo "   Space  - Pause/Resume"
echo "   Q/ESC  - Quit"
echo "   ←/→    - Seek ±10 seconds"
echo "   F      - Fullscreen"
echo ""
echo "Ready to test? Run:"
echo "  ./ffplay h3://120.79.21.28:8443/600k_cbr.flv"
echo ""
