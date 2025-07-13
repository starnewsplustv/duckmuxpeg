#!/bin/bash

# DuckMuxPeg 2025 - Quantum Matrix Interface Launcher
# Advanced startup script with dependency checking and optimization

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ASCII Art Banner
print_banner() {
    echo -e "${CYAN}"
    cat << "EOF"
    ╔══════════════════════════════════════════════════════╗
    ║                                                      ║
    ║    ██████╗ ██╗   ██╗ ██████╗██╗  ██╗███╗   ███╗    ║
    ║    ██╔══██╗██║   ██║██╔════╝██║ ██╔╝████╗ ████║    ║
    ║    ██║  ██║██║   ██║██║     █████╔╝ ██╔████╔██║    ║
    ║    ██║  ██║██║   ██║██║     ██╔═██╗ ██║╚██╔╝██║    ║
    ║    ██████╔╝╚██████╔╝╚██████╗██║  ██╗██║ ╚═╝ ██║    ║
    ║    ╚═════╝  ╚═════╝  ╚═════╝╚═╝  ╚═╝╚═╝     ╚═╝    ║
    ║                                                      ║
    ║         🌌 QUANTUM MATRIX INTERFACE 2025 🌌          ║
    ║                                                      ║
    ╚══════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

# Check if Node.js is installed
check_node() {
    echo -e "${BLUE}🔍 Checking Node.js installation...${NC}"
    
    if ! command -v node &> /dev/null; then
        echo -e "${RED}❌ Node.js is not installed!${NC}"
        echo -e "${YELLOW}Please install Node.js 18+ from: https://nodejs.org/${NC}"
        exit 1
    fi
    
    NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
    if [ "$NODE_VERSION" -lt 18 ]; then
        echo -e "${RED}❌ Node.js version $NODE_VERSION detected. Requires Node.js 18+${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Node.js $(node -v) detected${NC}"
}

# Check if npm is available
check_npm() {
    echo -e "${BLUE}🔍 Checking npm installation...${NC}"
    
    if ! command -v npm &> /dev/null; then
        echo -e "${RED}❌ npm is not installed!${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ npm $(npm -v) detected${NC}"
}

# Check system resources
check_system() {
    echo -e "${BLUE}🔍 Checking system requirements...${NC}"
    
    # Check available memory (Linux/macOS)
    if command -v free &> /dev/null; then
        MEMORY_GB=$(free -g | awk '/^Mem:/{print $2}')
        if [ "$MEMORY_GB" -lt 4 ]; then
            echo -e "${YELLOW}⚠️  Warning: Only ${MEMORY_GB}GB RAM detected. 4GB+ recommended for optimal performance.${NC}"
        else
            echo -e "${GREEN}✅ Memory: ${MEMORY_GB}GB${NC}"
        fi
    elif command -v system_profiler &> /dev/null; then
        # macOS memory check
        MEMORY_GB=$(system_profiler SPHardwareDataType | grep "Memory:" | awk '{print $2}' | cut -d' ' -f1)
        echo -e "${GREEN}✅ Memory: ${MEMORY_GB}${NC}"
    fi
    
    # Check if running in suitable environment
    if [[ "$TERM" == *"color"* ]] || [[ "$COLORTERM" != "" ]]; then
        echo -e "${GREEN}✅ Color terminal support detected${NC}"
    fi
}

# Install dependencies
install_dependencies() {
    echo -e "${BLUE}📦 Installing dependencies...${NC}"
    
    if [ ! -d "node_modules" ]; then
        echo -e "${YELLOW}⏳ First-time setup - this may take a few minutes...${NC}"
        npm install --silent
        echo -e "${GREEN}✅ Dependencies installed successfully${NC}"
    else
        echo -e "${CYAN}🔄 Checking for updates...${NC}"
        npm install --silent
        echo -e "${GREEN}✅ Dependencies updated${NC}"
    fi
}

# Optimize for development
optimize_dev() {
    echo -e "${BLUE}⚡ Optimizing development environment...${NC}"
    
    # Set Node.js memory limit for large projects
    export NODE_OPTIONS="--max-old-space-size=4096"
    
    # Enable performance monitoring
    export NODE_ENV="development"
    
    echo -e "${GREEN}✅ Development environment optimized${NC}"
}

# Check browser compatibility
check_browser() {
    echo -e "${BLUE}🌐 Browser compatibility information:${NC}"
    echo -e "${CYAN}   • Chrome/Chromium 90+ (Recommended)${NC}"
    echo -e "${CYAN}   • Firefox 88+ (Good performance)${NC}"
    echo -e "${CYAN}   • Safari 14+ (Basic support)${NC}"
    echo -e "${CYAN}   • Edge 90+ (Good performance)${NC}"
    echo -e "${YELLOW}   ⚠️  Requires WebGL support for 3D effects${NC}"
}

# Launch interface
launch_interface() {
    echo -e "${PURPLE}🚀 Launching Quantum Matrix Interface...${NC}"
    echo -e "${CYAN}   Interface will be available at: http://localhost:3000${NC}"
    echo -e "${CYAN}   Press Ctrl+C to stop the development server${NC}"
    echo ""
    
    # Start development server
    npm run dev
}

# Performance tips
show_performance_tips() {
    echo -e "${YELLOW}💡 Performance Tips:${NC}"
    echo -e "${CYAN}   • Use Chrome for best WebGL performance${NC}"
    echo -e "${CYAN}   • Close unnecessary browser tabs${NC}"
    echo -e "${CYAN}   • Update graphics drivers for optimal rendering${NC}"
    echo -e "${CYAN}   • Use 'Reduce Effects' button if experiencing lag${NC}"
    echo ""
}

# Main execution
main() {
    clear
    print_banner
    
    echo -e "${PURPLE}🌌 Initializing Quantum Matrix Interface...${NC}"
    echo ""
    
    # Run all checks
    check_node
    check_npm
    check_system
    
    echo ""
    echo -e "${PURPLE}🔧 Setting up environment...${NC}"
    
    # Setup
    install_dependencies
    optimize_dev
    
    echo ""
    check_browser
    echo ""
    show_performance_tips
    
    # Launch
    launch_interface
}

# Error handling
trap 'echo -e "\n${RED}❌ Interface startup interrupted${NC}"; exit 1' INT TERM

# Run main function
main "$@"