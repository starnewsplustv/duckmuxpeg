#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PREFIX=${INSTALL_PREFIX:-${1:-/usr/local}}
BUILD_TYPE=${BUILD_TYPE:-Release}

printf '\nDuckMuxPeg installation\n'
printf '======================\n'
printf 'Prefix: %s\n' "$PREFIX"
printf 'Build type: %s\n' "$BUILD_TYPE"

INSTALL_PREFIX="$PREFIX" BUILD_TYPE="$BUILD_TYPE" "$SCRIPT_DIR/build.sh" build

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory was not created; aborting install" >&2
    exit 1
fi

cmake --install "$BUILD_DIR" --prefix "$PREFIX"

echo "\nInstallation complete. Binaries installed to $PREFIX/bin."
