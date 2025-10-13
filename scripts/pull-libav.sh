#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOWNLOAD_DIR="${ROOT_DIR}/third_party/libav/downloads"
STAGING_DIR="${ROOT_DIR}/third_party/libav/root"

PACKAGES=(
    libavcodec-dev
    libavformat-dev
    libavutil-dev
    libswscale-dev
    libswresample-dev
)

usage() {
    cat <<USAGE
Usage: ${0##*/} [--no-update]

Fetches Debian/Ubuntu libav (FFmpeg) development packages without root
privileges and extracts them into third_party/libav. The generated tree
contains include files, shared libraries, and pkg-config metadata that can be
picked up automatically by CMake.

Options:
  --no-update    Skip running 'apt-get update' before downloading packages.
USAGE
}

run_update=1
if [[ ${1-} == "--help" ]]; then
    usage
    exit 0
elif [[ ${1-} == "--no-update" ]]; then
    run_update=0
fi

command -v apt-get >/dev/null 2>&1 || {
    echo "apt-get is required to download the libav packages" >&2
    exit 1
}
command -v dpkg-deb >/dev/null 2>&1 || {
    echo "dpkg-deb is required to extract the libav packages" >&2
    exit 1
}

mkdir -p "${DOWNLOAD_DIR}" "${STAGING_DIR}"
rm -f "${DOWNLOAD_DIR}"/*.deb 2>/dev/null || true

if [[ $run_update -eq 1 ]]; then
    echo "Updating package index..."
    if command -v sudo >/dev/null 2>&1 && sudo -n true >/dev/null 2>&1; then
        sudo apt-get update
    elif [[ $(id -u) -eq 0 ]]; then
        apt-get update
    else
        echo "  (skipping: no passwordless sudo; run with sudo or pass --no-update)"
    fi
fi

echo "Downloading libav development packages..."
for pkg in "${PACKAGES[@]}"; do
    echo "  - ${pkg}"
    (cd "${DOWNLOAD_DIR}" && apt-get download "$pkg")
    echo
done

echo "Extracting packages into ${STAGING_DIR}..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}"

for deb in "${DOWNLOAD_DIR}"/*.deb; do
    echo "  * ${deb##*/}"
    dpkg-deb -x "$deb" "${STAGING_DIR}"
done

echo
cat <<INFO
libav development files extracted.
Add the following environment variables before configuring CMake:

    export PKG_CONFIG_PATH="${STAGING_DIR}/usr/lib/x86_64-linux-gnu/pkgconfig:${STAGING_DIR}/usr/lib/pkgconfig:\${PKG_CONFIG_PATH:-}"
    export LD_LIBRARY_PATH="${STAGING_DIR}/usr/lib/x86_64-linux-gnu:${STAGING_DIR}/usr/lib:\${LD_LIBRARY_PATH:-}"
    export CMAKE_PREFIX_PATH="${STAGING_DIR}/usr:\${CMAKE_PREFIX_PATH:-}"

CMake will automatically detect these packages when building DuckMuxPeg.
INFO
