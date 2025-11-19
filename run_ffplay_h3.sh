#!/bin/bash
#
# Wrapper script to run our custom ffplay with H3 support
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FFPLAY="$SCRIPT_DIR/ffplay"

if [ ! -f "$FFPLAY" ]; then
    echo "Error: ffplay not found at $FFPLAY"
    echo "Please run build_ffmpeg_with_h3_v2.sh first"
    exit 1
fi

echo "=== FFplay with H3/QUIC Support ==="
echo "Binary: $FFPLAY"
echo "Protocols: $($FFPLAY -protocols 2>&1 | grep -E '(h3|http)')"
echo ""

# Check if URL provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <url> [ffplay options]"
    echo ""
    echo "Examples:"
    echo "  $0 h3://120.79.21.28:8443/600k_cbr.flv"
    echo "  $0 h3://example.com/video.mp4 -loglevel debug"
    echo "  $0 https://120.79.21.28:8443/600k_cbr.flv"
    echo ""
    exit 1
fi

# Run ffplay with provided arguments
echo "Running: $FFPLAY $@"
echo ""
exec "$FFPLAY" "$@"
