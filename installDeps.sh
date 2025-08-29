#!/bin/bash

# Remote Desktop Dependencies Installation Script
# This script automatically installs all required dependencies for the remote-desk project

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to detect OS
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [ -f /etc/redhat-release ]; then
        OS="Red Hat Enterprise Linux"
        VER=$(cat /etc/redhat-release | sed 's/.*release //' | sed 's/ .*//')
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
}

# Function to install dependencies on Ubuntu/Debian
install_ubuntu_debian() {
    print_info "Installing dependencies for Ubuntu/Debian..."
    
    # Update package list
    print_info "Updating package list..."
    sudo apt-get update
    
    # Install build tools
    print_info "Installing build tools..."
    sudo apt-get install -y build-essential cmake pkg-config
    
    # Install X11 libraries
    print_info "Installing X11 development libraries..."
    sudo apt-get install -y libx11-dev libxext-dev
    
    # Install virtual display (optional)
    print_info "Installing virtual display support (Xvfb)..."
    sudo apt-get install -y xvfb
    
    print_success "All dependencies installed successfully for Ubuntu/Debian!"
}

# Function to install dependencies on CentOS/RHEL/Fedora
install_redhat_fedora() {
    print_info "Installing dependencies for CentOS/RHEL/Fedora..."
    
    # Detect package manager
    if command -v dnf >/dev/null 2>&1; then
        PKG_MGR="dnf"
    elif command -v yum >/dev/null 2>&1; then
        PKG_MGR="yum"
    else
        print_error "Neither dnf nor yum package manager found!"
        exit 1
    fi
    
    print_info "Using package manager: $PKG_MGR"
    
    # Install build tools
    print_info "Installing build tools..."
    sudo $PKG_MGR install -y gcc-c++ cmake pkgconfig
    
    # Install X11 libraries
    print_info "Installing X11 development libraries..."
    sudo $PKG_MGR install -y libX11-devel libXext-devel
    
    # Install virtual display (optional)
    print_info "Installing virtual display support (Xvfb)..."
    sudo $PKG_MGR install -y xorg-x11-server-Xvfb
    
    print_success "All dependencies installed successfully for CentOS/RHEL/Fedora!"
}

# Function to show usage instructions
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  --no-virtual   Skip virtual display (Xvfb) installation"
    echo "  --dry-run      Show what would be installed without actually installing"
    echo ""
    echo "This script automatically detects your Linux distribution and installs"
    echo "all required dependencies for the remote-desk project."
}

# Parse command line arguments
INSTALL_VIRTUAL=true
DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        --no-virtual)
            INSTALL_VIRTUAL=false
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Main installation process
main() {
    print_info "Remote Desktop Dependencies Installation Script"
    print_info "=============================================="
    
    # Detect OS
    detect_os
    print_info "Detected OS: $OS"
    
    if [ "$DRY_RUN" = true ]; then
        print_warning "DRY RUN MODE - No packages will be installed"
    fi
    
    # Install based on OS
    case $OS in
        *"Ubuntu"*|*"Debian"*)
            if [ "$DRY_RUN" = false ]; then
                install_ubuntu_debian
            else
                print_info "Would install packages for Ubuntu/Debian"
            fi
            ;;
        *"CentOS"*|*"Red Hat"*|*"Fedora"*)
            if [ "$DRY_RUN" = false ]; then
                install_redhat_fedora
            else
                print_info "Would install packages for CentOS/RHEL/Fedora"
            fi
            ;;
        *)
            print_error "Unsupported operating system: $OS"
            print_error "This script supports Ubuntu, Debian, CentOS, RHEL, and Fedora"
            exit 1
            ;;
    esac
    
    if [ "$DRY_RUN" = false ]; then
        print_success "Installation completed successfully!"
        print_info ""
        print_info "Next steps:"
        print_info "1. Navigate to the project directory"
        print_info "2. Create build directory: mkdir build && cd build"
        print_info "3. Configure project: cmake .."
        print_info "4. Build project: make"
        print_info ""
        print_info "For headless environments, you can start a virtual display:"
        print_info "  Xvfb :99 -screen 0 1024x768x24 &"
        print_info "  export DISPLAY=:99"
    fi
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    print_warning "Running as root. This script will use sudo for package installation."
fi

# Run main function
main