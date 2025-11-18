#!/bin/bash
#
# Build FFmpeg with QuicheEngine H3 Protocol Support (V2 - Adapter based)
#
# This version uses the h3_adapter directory which contains all adaptation code
# without modifying QuicheEngine core files.
#
# Dependencies:
#   - h3_adapter/quiche_engine_c_api.h/cpp (C API wrapper)
#   - h3_adapter/ffmpeg_h3_protocol.c (FFmpeg protocol plugin)
#   - quiche/engine/include/quiche_engine.h (QuicheEngine C++ API)
#   - lib/macos/x86_64/libquicheengine.a (QuicheEngine library)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

echo_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PLAYER_DIR="$(cd .. && pwd)"
THIRD_PARTY_DIR="$PLAYER_DIR/3rd"
H3_ADAPTER_DIR="$SCRIPT_DIR/h3_adapter"
ENGINE_INCLUDE="$THIRD_PARTY_DIR/include"
QUICHE_ENGINE_LIB="$THIRD_PARTY_DIR/lib/libquicheengine.a"

echo_info "================================================================"
echo_info "  FFmpeg with H3 Protocol Build Script (V2 - Adapter Based)"
echo_info "================================================================"
echo_info ""
echo_info "Player Dir:       $PLAYER_DIR"
echo_info "3rd Party Dir:    $THIRD_PARTY_DIR"
echo_info "H3 Adapter:       $H3_ADAPTER_DIR"
echo_info "Engine Headers:   $ENGINE_INCLUDE"
echo_info "Engine Library:   $QUICHE_ENGINE_LIB"
echo_info ""

# ============================================================================
# Step 1: Check prerequisites
# ============================================================================
echo_step "Step 1: Checking prerequisites..."

if [ ! -d "$H3_ADAPTER_DIR" ]; then
    echo_error "H3 adapter directory not found: $H3_ADAPTER_DIR"
    exit 1
fi

if [ ! -f "$H3_ADAPTER_DIR/quiche_engine_c_api.h" ]; then
    echo_error "C API header not found: $H3_ADAPTER_DIR/quiche_engine_c_api.h"
    exit 1
fi

if [ ! -f "$H3_ADAPTER_DIR/quiche_engine_c_api.cpp" ]; then
    echo_error "C API implementation not found: $H3_ADAPTER_DIR/quiche_engine_c_api.cpp"
    exit 1
fi

if [ ! -f "$H3_ADAPTER_DIR/ffmpeg_h3_protocol.c" ]; then
    echo_error "FFmpeg protocol plugin not found: $H3_ADAPTER_DIR/ffmpeg_h3_protocol.c"
    exit 1
fi

if [ ! -f "$ENGINE_INCLUDE/quiche_engine.h" ]; then
    echo_error "QuicheEngine header not found: $ENGINE_INCLUDE/quiche_engine.h"
    exit 1
fi

if [ ! -f "$QUICHE_ENGINE_LIB" ]; then
    echo_error "QuicheEngine library not found: $QUICHE_ENGINE_LIB"
    echo_error "Please copy the library to 3rd party directory:"
    echo_error "  mkdir -p $THIRD_PARTY_DIR/lib"
    echo_error "  cp /path/to/lib/macos/x86_64/libquicheengine.a $THIRD_PARTY_DIR/lib/"
    exit 1
fi

echo_info "✓ All prerequisites met"
echo_info ""

# ============================================================================
# Step 2: Build H3 Adapter C API library
# ============================================================================
echo_step "Step 2: Building H3 Adapter C API library..."

cd "$H3_ADAPTER_DIR"
if [ -f "Makefile" ]; then
    echo_info "Using Makefile to build adapter..."
    make clean || true
    make
    H3_ADAPTER_LIB="$H3_ADAPTER_DIR/build/libh3adapter.a"
else
    echo_info "Building adapter manually..."
    mkdir -p build

    clang++ -std=c++11 -O2 -Wall -fPIC \
        -I. -I"$ENGINE_INCLUDE" \
        -c quiche_engine_c_api.cpp \
        -o build/quiche_engine_c_api.o

    ar rcs build/libh3adapter.a build/quiche_engine_c_api.o
    H3_ADAPTER_LIB="$H3_ADAPTER_DIR/build/libh3adapter.a"
fi

if [ ! -f "$H3_ADAPTER_LIB" ]; then
    echo_error "Failed to build H3 adapter library"
    exit 1
fi

echo_info "✓ H3 Adapter library built: $H3_ADAPTER_LIB"
ls -lh "$H3_ADAPTER_LIB"
echo_info ""

cd "$SCRIPT_DIR"

# ============================================================================
# Step 3: Integrate FFmpeg protocol plugin
# ============================================================================
echo_step "Step 3: Integrating FFmpeg protocol plugin..."

# Copy protocol plugin to FFmpeg's libavformat directory
if [ ! -d "libavformat" ]; then
    echo_error "libavformat directory not found. Are you in the FFmpeg source directory?"
    exit 1
fi

echo_info "Copying protocol plugin to libavformat/quiche.c..."
cp "$H3_ADAPTER_DIR/ffmpeg_h3_protocol.c" libavformat/quiche.c

# Check if quiche.o is in Makefile
if ! grep -q "quiche.o" libavformat/Makefile 2>/dev/null; then
    echo_warn "quiche.o not found in libavformat/Makefile"
    echo_info "Adding quiche.o to Makefile..."

    # Backup Makefile
    cp libavformat/Makefile libavformat/Makefile.bak

    # Add quiche.o after prompeg.o
    sed -i.tmp '/^OBJS.*=.*\\$/,/^$/s/prompeg\.o,/prompeg.o, \\\n       quiche.o,/' libavformat/Makefile
    rm -f libavformat/Makefile.tmp

    echo_info "✓ Added quiche.o to Makefile"
else
    echo_info "✓ quiche.o already in Makefile"
fi

# Check protocol registration
if ! grep -q "ff_h3_protocol" libavformat/allformats.c 2>/dev/null; then
    echo_warn "ff_h3_protocol not registered in allformats.c"
    echo_warn "You need to manually add to libavformat/allformats.c:"
    echo_warn "  1. Add declaration: extern const URLProtocol ff_h3_protocol;"
    echo_warn "  2. Add to url_protocols array: &ff_h3_protocol,"
else
    echo_info "✓ Protocol already registered in allformats.c"
fi

echo_info ""

# ============================================================================
# Step 4: Configure FFmpeg
# ============================================================================
echo_step "Step 4: Configuring FFmpeg with H3 protocol..."

# Clean previous configuration
if [ -f "ffbuild/config.mak" ]; then
    echo_info "Cleaning previous configuration..."
    make clean || true
fi

echo_info "Running ./configure..."

./configure \
    --enable-static \
    --disable-shared \
    --enable-ffplay \
    --disable-ffmpeg \
    --disable-ffprobe \
    --disable-doc \
    --disable-x86asm \
    --enable-sdl2 \
    --extra-cflags="-I$H3_ADAPTER_DIR -I$ENGINE_INCLUDE" \
    --extra-ldflags="-L$(dirname $QUICHE_ENGINE_LIB) -L$(dirname $H3_ADAPTER_LIB)" \
    --extra-libs="-lquicheengine -lstdc++ -lpthread" \
    --enable-protocol=h3

if [ $? -ne 0 ]; then
    echo_error "Configuration failed!"
    exit 1
fi

echo_info "✓ Configuration completed"
echo_info ""

# ============================================================================
# Step 5: Build FFmpeg
# ============================================================================
echo_step "Step 5: Building FFmpeg..."

# Get number of CPU cores
if [[ "$OSTYPE" == "darwin"* ]]; then
    NCPU=$(sysctl -n hw.ncpu)
else
    NCPU=$(nproc)
fi

echo_info "Building with $NCPU parallel jobs..."

make -j${NCPU}

if [ $? -ne 0 ]; then
    echo_error "Build failed!"
    exit 1
fi

echo_info "✓ Build completed"
echo_info ""

# ============================================================================
# Step 6: Verify
# ============================================================================
echo_step "Step 6: Verifying build..."

if [ ! -f "ffplay" ]; then
    echo_error "ffplay binary not found!"
    exit 1
fi

echo_info "✓ Binary size: $(du -h ffplay | cut -f1)"
echo_info ""

# Check for h3 protocol
echo_info "Checking protocol support..."
if ./ffplay -protocols 2>&1 | grep -q "h3"; then
    echo_info "✓ H3 protocol is registered"
else
    echo_warn "⚠ H3 protocol not found in protocol list"
    echo_warn "This may be because allformats.c was not updated."
fi

echo_info ""
echo_info "================================================================"
echo_info "  Build Summary"
echo_info "================================================================"
echo_info "✓ H3 Adapter library built"
echo_info "✓ FFmpeg protocol plugin integrated"
echo_info "✓ FFmpeg compiled successfully"
echo_info "✓ Binary location: $SCRIPT_DIR/ffplay"
echo_info ""
echo_info "Adapter Files:"
echo_info "  - C API Header:     $H3_ADAPTER_DIR/quiche_engine_c_api.h"
echo_info "  - C API Impl:       $H3_ADAPTER_DIR/quiche_engine_c_api.cpp"
echo_info "  - FFmpeg Plugin:    $H3_ADAPTER_DIR/ffmpeg_h3_protocol.c"
echo_info "  - Adapter Library:  $H3_ADAPTER_LIB"
echo_info ""
echo_info "Usage:"
echo_info "  ./ffplay h3://example.com/video.mp4"
echo_info ""
echo_info "Testing:"
echo_info "  ./ffplay -protocols | grep h3"
echo_info "  ./ffplay -loglevel debug h3://your-server/test.mp4"
echo_info ""
echo_warn "Note: Make sure your server supports HTTP/3!"
echo_info ""
