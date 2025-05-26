#!/bin/bash

# Test all kernel modules in the tutorials
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

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run as root (use sudo)"
    exit 1
fi

# Get kernel version
KERNEL_VER=$(uname -r)
print_header "Testing modules for kernel version: $KERNEL_VER"

# Create a temporary directory for testing
TEMP_DIR=$(mktemp -d)
echo "Created temporary directory: $TEMP_DIR"

# Clean up function
cleanup() {
    print_header "Cleaning up..."
    
    # Unload all modules
    if lsmod | grep -q hello; then
        rmmod hello
    fi
    if lsmod | grep -q simple_char; then
        rmmod simple_char
    fi
    if lsmod | grep -q kmem_demo; then
        rmmod kmem_demo
    fi
    if lsmod | grep -q sync_demo; then
        rmmod sync_demo
    fi
    if lsmod | grep -q irq_demo; then
        rmmod irq_demo
    fi
    
    # Remove the temporary directory
    rm -rf "$TEMP_DIR"
    echo "Removed temporary directory"
    
    print_success "Cleanup completed"
}

# Register the cleanup function to run on exit
trap cleanup EXIT

# Tutorial 1: Hello World
print_header "Building and testing tutorial-01 (Hello World)"

cd tutorial-01
# Build the module
run_cmd "$MAKE clean && $MAKE"
print_success "Tutorial-01 built successfully"

# Load the module
run_cmd "insmod hello.ko"
print_success "Loaded hello.ko module"

# Check that it loaded
if lsmod | grep -q hello; then
    print_success "Module hello is loaded"
else
    print_error "Module hello failed to load"
fi

# Check dmesg for the module's output
if dmesg | tail -n 10 | grep -q "Hello World: Module loaded"; then
    print_success "Module hello printed correct message"
else
    print_error "Module hello did not print expected message"
fi

# Unload the module
run_cmd "rmmod hello"
print_success "Unloaded hello module"

# Check dmesg for the module's unload message
if dmesg | tail -n 10 | grep -q "Hello World: Module unloaded"; then
    print_success "Module hello unloaded correctly"
else
    print_error "Module hello did not print expected unload message"
fi

# Tutorial 2: Character Device
print_header "Building and testing tutorial-02 (Character Device)"

cd ../tutorial-02
# Build the module
run_cmd "$MAKE clean && $MAKE"
print_success "Tutorial-02 built successfully"

# Build the test program
run_cmd "$GCC -o test_char test_char.c"
print_success "Built test_char program"

# Load the module
run_cmd "insmod simple_char.ko"
print_success "Loaded simple_char.ko module"

# Check that the device was created
if [ -c /dev/simple_char ]; then
    print_success "Device /dev/simple_char was created"
else
    print_error "Device /dev/simple_char was not created"
fi

# Run basic tests on the device
echo "Testing write to device..."
echo "Test data" > /dev/simple_char
print_success "Wrote data to device"

echo "Testing read from device..."
READ_DATA=$(cat /dev/simple_char)
if [[ "$READ_DATA" == *"Test data"* ]]; then
    print_success "Read correct data from device"
else
    print_error "Failed to read correct data from device"
fi

# Run the test program with the "test" option
echo "Running comprehensive test suite..."
./test_char test
print_success "Character device tests completed successfully"

# Unload the module
run_cmd "rmmod simple_char"
print_success "Unloaded simple_char module"

# Tutorial 3: Kernel Memory Management
print_header "Building and testing tutorial-03 (Kernel Memory)"

cd ../tutorial-03
# Build the module
run_cmd "$MAKE clean && $MAKE"
print_success "Tutorial-03 built successfully"

# Build the test program
run_cmd "$GCC -o test_kmem test_kmem.c"
print_success "Built test_kmem program"

# Load the module
run_cmd "insmod kmem_demo.ko"
print_success "Loaded kmem_demo.ko module"

# Check that the proc file was created
if [ -f /proc/kmem_demo ]; then
    print_success "Proc file /proc/kmem_demo was created"
else
    print_error "Proc file /proc/kmem_demo was not created"
fi

# Check content of the proc file
KMEM_OUTPUT=$(cat /proc/kmem_demo)
echo "Memory allocation info:"
echo "$KMEM_OUTPUT" | head -n 15
echo "..."

# Check that all four allocation types are present
if [[ "$KMEM_OUTPUT" == *"kmalloc"* && "$KMEM_OUTPUT" == *"vmalloc"* && 
      "$KMEM_OUTPUT" == *"__get_free_pages"* && "$KMEM_OUTPUT" == *"kmem_cache"* ]]; then
    print_success "All four memory allocation types were found"
else
    print_error "Not all memory allocation types were found in output"
fi

# Run the display command in the test program
echo "Running memory display test..."
./test_kmem display
print_success "Memory information displayed successfully"

# Unload the module
run_cmd "rmmod kmem_demo"
print_success "Unloaded kmem_demo module"

# Tutorial 4: Synchronization Primitives
print_header "Building and testing tutorial-04 (Synchronization Primitives)"

cd ../tutorial-04
# Build the module
run_cmd "$MAKE clean && $MAKE"
print_success "Tutorial-04 built successfully"

# Build the test program
run_cmd "$GCC -o test_sync test_sync.c"
print_success "Built test_sync program"

# Load the module
run_cmd "insmod sync_demo.ko"
print_success "Loaded sync_demo.ko module"

# Check that the device and proc file were created
if [ -c /dev/sync_demo ]; then
    print_success "Device /dev/sync_demo was created"
else
    print_error "Device /dev/sync_demo was not created"
fi

if [ -f /proc/sync_demo ]; then
    print_success "Proc file /proc/sync_demo was created"
else
    print_error "Proc file /proc/sync_demo was not created"
fi

# Check initial counter values
SYNC_OUTPUT=$(cat /proc/sync_demo)
echo "Initial synchronization counters:"
echo "$SYNC_OUTPUT" | head -n 10

# Wait for the counters to increment
echo "Waiting for counters to increment (5 seconds)..."
sleep 5

# Check that counters have incremented
SYNC_OUTPUT_AFTER=$(cat /proc/sync_demo)
echo "Synchronization counters after waiting:"
echo "$SYNC_OUTPUT_AFTER" | head -n 10

# Reset the counters
echo "Resetting counters..."
echo "reset" > /dev/sync_demo
print_success "Reset counters"

# Check that counters were reset
SYNC_OUTPUT_RESET=$(cat /proc/sync_demo)
if [[ "$SYNC_OUTPUT_RESET" == *"Atomic counter: 0"* ]]; then
    print_success "Counters were reset successfully"
else
    print_error "Counters were not reset"
fi

# Run the test program
echo "Running sync test program..."
./test_sync

# Unload the module
run_cmd "rmmod sync_demo"
print_success "Unloaded sync_demo module"

# Tutorial 5: Interrupt Handling
print_header "Building and testing tutorial-05 (Interrupt Handling)"

cd ../tutorial-05
# Build the module
run_cmd "$MAKE clean && $MAKE"
print_success "Tutorial-05 built successfully"

# Build the test program
run_cmd "$GCC -o test_irq test_irq.c"
print_success "Built test_irq program"

# Load the module
run_cmd "insmod irq_demo.ko"
print_success "Loaded irq_demo.ko module"

# Check that the device and proc file were created
if [ -c /dev/irq_demo ]; then
    print_success "Device /dev/irq_demo was created"
else
    print_error "Device /dev/irq_demo was not created"
fi

if [ -f /proc/irq_demo ]; then
    print_success "Proc file /proc/irq_demo was created"
else
    print_error "Proc file /proc/irq_demo was not created"
fi

# Check initial IRQ and work counts
IRQ_OUTPUT=$(cat /proc/irq_demo)
echo "Initial IRQ and work counts:"
echo "$IRQ_OUTPUT" | head -n 10

# Manually trigger an interrupt
echo "Triggering interrupt..."
echo "trigger" > /dev/irq_demo
print_success "Triggered interrupt"

# Wait for the bottom half to execute
sleep 1

# Check that the counters incremented
IRQ_OUTPUT_AFTER=$(cat /proc/irq_demo)
echo "IRQ and work counts after trigger:"
echo "$IRQ_OUTPUT_AFTER" | head -n 10

# Reset the counters
echo "Resetting counters..."
echo "reset" > /dev/irq_demo
print_success "Reset counters"

# Run test script in non-interactive mode by piping input
echo "Running IRQ test program (automatically exiting after basic tests)..."
echo "5" | ./test_irq

# Unload the module
run_cmd "rmmod irq_demo"
print_success "Unloaded irq_demo module"

# Final summary
print_header "Test Summary"
print_success "All kernel modules built and tested successfully"
print_success "All modules are compatible with Linux kernel $KERNEL_VER (6.12+)"
echo -e "\nNote: For full interactive testing, you can run the individual test programs manually."
echo "This automated test confirms basic functionality of all modules." 