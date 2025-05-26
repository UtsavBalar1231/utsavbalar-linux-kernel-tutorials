# Kernel Module Testing Guide

This directory contains scripts to build and test all kernel modules in the tutorials.

## Prerequisites

- Linux kernel headers (matching your running kernel)
- Build tools: gcc, make
- Root access (for loading/unloading modules)

## Build Script

The `build_all.sh` script compiles all kernel modules without loading them.

```bash
./build_all.sh
```

This script:
- Builds all modules in each tutorial directory
- Reports any build errors
- Confirms compatibility with Linux kernel 6.12+

## Test Script

The `test_all.sh` script automatically:
1. Builds all kernel modules
2. Loads each module into the kernel
3. Tests the functionality of each module
4. Unloads the modules and cleans up

Since loading kernel modules requires root privileges, run:

```bash
sudo ./test_all.sh
```

### Test Script Details

The test script runs these steps for each module:
- Compiles the module
- Compiles the test program
- Loads the module into the kernel
- Verifies the module loaded correctly (checks device nodes, proc entries)
- Tests basic module functionality
- Runs the user-space test program in non-interactive mode
- Unloads the module
- Performs cleanup

A trap handler ensures all modules are unloaded even if the script exits early due to an error.

## Manual Testing

After confirming the modules build and load correctly with the automated tests, you can perform more in-depth manual testing:

### Hello World Module (tutorial-01)
```bash
cd tutorial-01
sudo insmod hello.ko
dmesg | tail
sudo rmmod hello
dmesg | tail
```

### Character Device (tutorial-02)
```bash
cd tutorial-02
sudo insmod simple_char.ko
./test_char test     # Run comprehensive test suite
sudo rmmod simple_char
```

### Kernel Memory Management (tutorial-03)
```bash
cd tutorial-03
sudo insmod kmem_demo.ko
cat /proc/kmem_demo
./test_kmem monitor  # Monitor memory allocations
sudo rmmod kmem_demo
```

### Synchronization Primitives (tutorial-04)
```bash
cd tutorial-04
sudo insmod sync_demo.ko
cat /proc/sync_demo
./test_sync          # Run interactive test
sudo rmmod sync_demo
```

### Interrupt Handling (tutorial-05)
```bash
cd tutorial-05
sudo insmod irq_demo.ko
cat /proc/irq_demo
./test_irq           # Run interactive test
sudo rmmod irq_demo
```

## Troubleshooting

If you encounter issues:

1. Check your kernel version: `uname -r`
2. Ensure you have the matching kernel headers installed
3. Check dmesg for kernel module errors: `dmesg | tail`
4. Ensure you're running the test script as root
5. If a module fails to unload, check if it's in use: `lsmod | grep <module_name>` 
