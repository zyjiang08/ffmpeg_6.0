#!/bin/bash
# FFplay launcher script with correct library paths

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

export DYLD_LIBRARY_PATH="${SCRIPT_DIR}/libavdevice:${SCRIPT_DIR}/libavformat:${SCRIPT_DIR}/libavcodec:${SCRIPT_DIR}/libavfilter:${SCRIPT_DIR}/libavutil:${SCRIPT_DIR}/libswscale:${SCRIPT_DIR}/libswresample"

exec ./ffplay "$@"
