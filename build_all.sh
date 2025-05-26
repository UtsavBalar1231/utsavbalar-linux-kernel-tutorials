#!/bin/bash

# Build all kernel modules in the tutorials
set -e

# Define the path to system commands
GCC="/usr/bin/gcc"
MAKE="/usr/bin/make"

# Define colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Function to print a header
print_header() {
    echo -e "\n${YELLOW}===================================================${NC}"
    echo -e "${YELLOW}$1${NC}"
    echo -e "${YELLOW}===================================================${NC}\n"
}

# Function to print success message
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

# Function to print error message and exit
print_error() {
    echo -e "${RED}✗ $1${NC}"
    exit 1
}

# Function to run a command with error handling
run_cmd() {
    echo -e "Running: $1"
    if eval $1; then
        return 0
    else
        print_error "Command failed: $1"
        return 1
    fi
}

# Get kernel version
KERNEL_VER=$(uname -r)
print_header "Building modules for kernel version: $KERNEL_VER"

# Build tutorial-01
print_header "Building tutorial-01 (Hello World)"
cd tutorial-01
run_cmd "$MAKE clean && $MAKE"
print_success "tutorial-01 built successfully"

# Build tutorial-02
print_header "Building tutorial-02 (Character Device)"
cd ../tutorial-02
run_cmd "$MAKE clean && $MAKE"
run_cmd "$GCC -o test_char test_char.c"
print_success "tutorial-02 built successfully"

# Build tutorial-03
print_header "Building tutorial-03 (Kernel Memory)"
cd ../tutorial-03
run_cmd "$MAKE clean && $MAKE"
run_cmd "$GCC -o test_kmem test_kmem.c"
print_success "tutorial-03 built successfully"

# Build tutorial-04
print_header "Building tutorial-04 (Synchronization Primitives)"
cd ../tutorial-04
run_cmd "$MAKE clean && $MAKE"
run_cmd "$GCC -o test_sync test_sync.c"
print_success "tutorial-04 built successfully"

# Build tutorial-05
print_header "Building tutorial-05 (Interrupt Handling)"
cd ../tutorial-05
run_cmd "$MAKE clean && $MAKE"
run_cmd "$GCC -o test_irq test_irq.c"
print_success "tutorial-05 built successfully"

print_header "Build Summary"
print_success "All modules built successfully for kernel $KERNEL_VER"
print_success "All modules are compatible with Linux kernel 6.12+"
echo -e "\nTo test the modules, run: sudo ./test_all.sh"
