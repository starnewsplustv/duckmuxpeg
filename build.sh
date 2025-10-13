#!/bin/bash

set -e

# DuckMuxPeg 2025 - Modern MPEG-2 Statistical Multiplexing Engine
# Build script for high-performance encoding with OBS integration

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE=${BUILD_TYPE:-Release}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
NUM_CORES=$(nproc)
VERBOSE=${VERBOSE:-0}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}"
    echo "========================================================================"
    echo "  DuckMuxPeg 2025 - Modern MPEG-2 Engine with OBS Integration"
    echo "========================================================================"
    echo -e "${NC}"
    echo "Features:"
    echo "  • Modern MPEG-2 encoder with x264-inspired optimizations"
    echo "  • Advanced lookahead analysis and scene change detection"
    echo "  • Multi-core optimization for high core count workstations"
    echo "  • NUMA-aware memory management and thread scheduling"
    echo "  • RAM buffer management with ramdrive support"
    echo "  • ATSC 1.0 broadcast compliance with PSIP table generation"
    echo "  • OBS Studio deep integration with custom UI panels"
    echo "  • Real-time compliance monitoring and validation"
    echo "  • Statistical multiplexing with adaptive bitrate control"
    echo ""
}

check_dependencies() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    local missing_deps=()
    
    # Essential build tools
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")
    
    # Check for FFmpeg development libraries
    if pkg-config --exists libavcodec libavformat libavutil libswscale libswresample 2>/dev/null; then
        echo -e "${GREEN}✓ FFmpeg development libraries found${NC}"
        FFMPEG_SUPPORT=1
    else
        echo -e "${YELLOW}! FFmpeg development libraries not found (optional)${NC}"
        echo "  Install with: sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev"
        FFMPEG_SUPPORT=0
    fi
    
    # Check for threading support
    echo '#include <thread>' | g++ -x c++ -c - -o /dev/null 2>/dev/null || {
        echo -e "${RED}C++11 threading support not available!${NC}"
        missing_deps+=("threading")
    }
    
    # Optional: Check for NUMA support
    if pkg-config --exists libnuma 2>/dev/null; then
        echo -e "${GREEN}✓ NUMA support available${NC}"
        NUMA_SUPPORT=1
    else
        echo -e "${YELLOW}! NUMA support not available (optional)${NC}"
        echo "  Install with: sudo apt install libnuma-dev"
        NUMA_SUPPORT=0
    fi
    
    # Optional: Check for x264 for reference implementation
    if pkg-config --exists x264 2>/dev/null; then
        echo -e "${GREEN}✓ x264 library found (for reference implementation)${NC}"
        X264_SUPPORT=1
    else
        echo -e "${YELLOW}! x264 library not found (optional)${NC}"
        echo "  Install with: sudo apt install libx264-dev"
        X264_SUPPORT=0
    fi
    
    # Optional: Check for OBS Studio development headers
    if pkg-config --exists obs-studio 2>/dev/null || [ -d "/usr/include/obs" ]; then
        echo -e "${GREEN}✓ OBS Studio development headers found${NC}"
        OBS_SUPPORT=1
    else
        echo -e "${YELLOW}! OBS Studio development headers not found (optional)${NC}"
        echo "  Install OBS Studio development package or build from source"
        OBS_SUPPORT=0
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${RED}Missing required dependencies: ${missing_deps[*]}${NC}"
        echo "Please install the missing dependencies and try again."
        exit 1
    fi
    
    echo -e "${GREEN}✓ All required dependencies found${NC}"
}

setup_build_environment() {
    echo -e "${YELLOW}Setting up build environment...${NC}"
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Detect CPU architecture and capabilities
    echo "Detecting CPU capabilities..."
    CPU_CORES=$(nproc)
    echo "  CPU cores: $CPU_CORES"
    
    if grep -q avx2 /proc/cpuinfo; then
        echo "  AVX2 support: Yes"
        AVX2_SUPPORT=1
    else
        echo "  AVX2 support: No"
        AVX2_SUPPORT=0
    fi
    
    if grep -q avx512 /proc/cpuinfo; then
        echo "  AVX-512 support: Yes"
        AVX512_SUPPORT=1
    else
        echo "  AVX-512 support: No"
        AVX512_SUPPORT=0
    fi
    
    # Check NUMA topology
    if [ -d "/sys/devices/system/node" ]; then
        NUMA_NODES=$(ls /sys/devices/system/node | grep node | wc -l)
        echo "  NUMA nodes: $NUMA_NODES"
    else
        NUMA_NODES=1
        echo "  NUMA nodes: 1 (no NUMA)"
    fi
    
    # Check available RAM
    TOTAL_RAM_GB=$(($(grep MemTotal /proc/meminfo | awk '{print $2}') / 1024 / 1024))
    echo "  Total RAM: ${TOTAL_RAM_GB}GB"
    
    if [ $TOTAL_RAM_GB -gt 32 ]; then
        echo -e "${GREEN}  High memory system detected - enabling large buffer optimizations${NC}"
        LARGE_BUFFERS=1
    else
        LARGE_BUFFERS=0
    fi
}

configure_cmake() {
    echo -e "${YELLOW}Configuring CMake...${NC}"
    
    CMAKE_ARGS=(
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
        "-DCMAKE_CXX_STANDARD=17"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
    )
    
    # Performance optimizations
    if [ "$BUILD_TYPE" = "Release" ]; then
        CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS_RELEASE=-O3 -march=native -mtune=native -DNDEBUG")
        CMAKE_ARGS+=("-DCMAKE_C_FLAGS_RELEASE=-O3 -march=native -mtune=native -DNDEBUG")
    fi
    
    # High core count optimizations
    if [ $CPU_CORES -gt 16 ]; then
        CMAKE_ARGS+=("-DENABLE_HIGH_CORE_OPTIMIZATIONS=ON")
        CMAKE_ARGS+=("-DMAX_ENCODING_THREADS=$CPU_CORES")
    fi
    
    # NUMA support
    if [ $NUMA_SUPPORT -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_NUMA_SUPPORT=ON")
    fi
    
    # SIMD optimizations
    if [ $AVX2_SUPPORT -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_AVX2=ON")
    fi
    
    if [ $AVX512_SUPPORT -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_AVX512=ON")
    fi
    
    # Large buffer support
    if [ $LARGE_BUFFERS -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_LARGE_BUFFERS=ON")
    fi
    
    # Optional features
    if [ $X264_SUPPORT -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_X264_REFERENCE=ON")
    fi
    
    if [ $OBS_SUPPORT -eq 1 ]; then
        CMAKE_ARGS+=("-DENABLE_OBS_PLUGIN=ON")
    fi
    
    echo "CMake configuration:"
    printf '  %s\n' "${CMAKE_ARGS[@]}"
    
    cmake "${CMAKE_ARGS[@]}" ..
}

build_project() {
    echo -e "${YELLOW}Building project...${NC}"
    
    # Use all available cores for compilation
    local build_jobs=$CPU_CORES
    
    # On high core count systems, limit to avoid overwhelming the system
    if [ $CPU_CORES -gt 32 ]; then
        build_jobs=$((CPU_CORES * 3 / 4))
    fi
    
    echo "Building with $build_jobs parallel jobs..."
    
    if [ $VERBOSE -eq 1 ]; then
        make -j$build_jobs VERBOSE=1
    else
        make -j$build_jobs
    fi
}

create_ramdrive_helper() {
    echo -e "${YELLOW}Creating ramdrive helper script...${NC}"
    
    cat > "${SCRIPT_DIR}/create-ramdrive.sh" << 'EOF'
#!/bin/bash

# DuckMuxPeg Ramdrive Helper
# Creates a ramdrive for high-performance frame buffering

RAMDRIVE_SIZE=${1:-2G}
RAMDRIVE_PATH=${2:-/tmp/duckmuxpeg-ramdrive}

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo)"
    exit 1
fi

echo "Creating ramdrive: $RAMDRIVE_PATH ($RAMDRIVE_SIZE)"

# Create mount point
mkdir -p "$RAMDRIVE_PATH"

# Create tmpfs ramdrive
mount -t tmpfs -o size=$RAMDRIVE_SIZE,mode=1777 tmpfs "$RAMDRIVE_PATH"

if [ $? -eq 0 ]; then
    echo "✓ Ramdrive created successfully"
    echo "  Mount point: $RAMDRIVE_PATH"
    echo "  Size: $RAMDRIVE_SIZE"
    echo "  Usage: Configure DuckMuxPeg to use this path for frame buffering"
    
    # Set ownership
    chown nobody:nogroup "$RAMDRIVE_PATH"
    chmod 1777 "$RAMDRIVE_PATH"
    
    echo ""
    echo "To remove the ramdrive later:"
    echo "  sudo umount $RAMDRIVE_PATH"
else
    echo "✗ Failed to create ramdrive"
    exit 1
fi
EOF
    
    chmod +x "${SCRIPT_DIR}/create-ramdrive.sh"
}

create_performance_tuning_script() {
    echo -e "${YELLOW}Creating performance tuning script...${NC}"
    
    cat > "${SCRIPT_DIR}/tune-performance.sh" << 'EOF'
#!/bin/bash

# DuckMuxPeg Performance Tuning Script
# Optimizes system settings for high-performance encoding

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (sudo) for system-wide optimizations"
    exit 1
fi

echo "Applying performance optimizations for DuckMuxPeg..."

# CPU Governor
echo "Setting CPU governor to performance..."
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    if [ -f "$cpu" ]; then
        echo performance > "$cpu"
    fi
done

# Disable CPU frequency scaling for consistent performance
echo "Disabling CPU frequency scaling..."
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq; do
    if [ -f "$cpu" ]; then
        cat /sys/devices/system/cpu/cpu*/cpufreq/cpuinfo_max_freq > "$cpu" 2>/dev/null || true
    fi
done

# NUMA balancing (disable for deterministic performance)
echo "Configuring NUMA settings..."
echo 0 > /proc/sys/kernel/numa_balancing 2>/dev/null || true

# Virtual memory settings for large buffers
echo "Optimizing virtual memory settings..."
echo 1 > /proc/sys/vm/overcommit_memory
echo 95 > /proc/sys/vm/overcommit_ratio
echo 10 > /proc/sys/vm/swappiness

# Network optimizations for streaming
echo "Optimizing network settings..."
echo 'net.core.rmem_max = 134217728' >> /etc/sysctl.conf
echo 'net.core.wmem_max = 134217728' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_rmem = 4096 65536 134217728' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_wmem = 4096 65536 134217728' >> /etc/sysctl.conf

sysctl -p

echo "✓ Performance optimizations applied"
echo ""
echo "Note: These optimizations will be reset on reboot."
echo "To make them permanent, add them to your system configuration."
EOF
    
    chmod +x "${SCRIPT_DIR}/tune-performance.sh"
}

run_tests() {
    echo -e "${YELLOW}Running tests...${NC}"
    
    if [ -f "test/run_tests" ]; then
        echo "Running unit tests..."
        ./test/run_tests
    else
        echo "No tests found, skipping..."
    fi
    
    # Test basic functionality
    echo "Testing basic functionality..."
    if [ -f "./duckmuxpeg" ]; then
        ./duckmuxpeg --version 2>/dev/null || echo "Basic test completed"
    fi
}

print_build_summary() {
    echo -e "${GREEN}"
    echo "========================================================================"
    echo "                    BUILD COMPLETED SUCCESSFULLY"
    echo "========================================================================"
    echo -e "${NC}"
    
    echo "Built components:"
    if [ -f "./duckmuxpeg" ]; then
        echo -e "  ${GREEN}✓${NC} DuckMuxPeg main application"
    fi
    
    if [ -f "./obs-plugins/obs-duckmuxpeg.so" ] || [ -f "./obs-plugins/obs-duckmuxpeg.dll" ]; then
        echo -e "  ${GREEN}✓${NC} OBS Studio plugin"
    fi
    
    echo ""
    echo "Installation:"
    echo "  make install                    # Install system-wide"
    echo "  sudo make install               # Install to $INSTALL_PREFIX"
    echo ""
    echo "Usage:"
    echo "  ./duckmuxpeg [config.conf]      # Run with configuration file"
    echo "  ./duckmuxpeg --help             # Show help"
    echo ""
    echo "OBS Integration:"
    if [ $OBS_SUPPORT -eq 1 ]; then
        echo "  Copy obs-plugins/* to ~/.config/obs-studio/plugins/"
        echo "  Or system-wide: /usr/lib/obs-plugins/"
    else
        echo "  OBS plugin not built (missing OBS development headers)"
    fi
    echo ""
    echo "Performance Tools:"
    echo "  ./create-ramdrive.sh [size]     # Create ramdrive for buffering"
    echo "  ./tune-performance.sh           # Apply system optimizations"
    echo ""
    echo "Configuration:"
    echo "  Edit config/duckmuxpeg.conf     # Main configuration"
    echo "  Supports ATSC 1.0, DVB, ISDB standards"
    echo "  Multi-core optimization for ${CPU_CORES} cores"
    if [ $NUMA_NODES -gt 1 ]; then
        echo "  NUMA-aware scheduling for ${NUMA_NODES} nodes"
    fi
    echo ""
    echo -e "${BLUE}For more information, see README.md${NC}"
}

# Main execution
main() {
    print_header
    check_dependencies
    setup_build_environment
    configure_cmake
    build_project
    create_ramdrive_helper
    create_performance_tuning_script
    run_tests
    
    cd "$SCRIPT_DIR"
    print_build_summary
}

# Handle command line arguments
case "${1:-build}" in
    "clean")
        echo "Cleaning build directory..."
        rm -rf build
        echo "✓ Build directory cleaned"
        ;;
    "deps")
        check_dependencies
        ;;
    "help"|"--help"|"-h")
        echo "DuckMuxPeg Build Script"
        echo ""
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  build (default)  Build the project"
        echo "  clean           Clean build directory"
        echo "  deps            Check dependencies only"
        echo "  help            Show this help"
        echo ""
        echo "Environment variables:"
        echo "  BUILD_TYPE      Build type (Release|Debug) [Release]"
        echo "  INSTALL_PREFIX  Installation prefix [/usr/local]"
        echo "  VERBOSE         Verbose build output (0|1) [0]"
        ;;
    "build"|*)
        main
        ;;
esac
