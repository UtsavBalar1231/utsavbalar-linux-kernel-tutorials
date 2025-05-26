# Linux Kernel Module Tutorials

This repository contains practical examples of Linux kernel modules corresponding to the tutorial series on [Utsav Balar's website](https://utsavbalar.com/tutorials).

## Tutorials

1. [Introduction to Linux Kernel Modules](./tutorial-01/) - Learn the fundamentals of Linux kernel modules and create your first 'Hello World' kernel module.
2. [Character Device Drivers Fundamentals](./tutorial-02/) - Master the essentials of character device drivers with a simple memory buffer device.
3. [Kernel Memory Management for Drivers](./tutorial-03/) - Explore various memory allocation techniques in the kernel with practical examples.
4. [Synchronization Primitives](./tutorial-04/) - Master essential synchronization mechanisms in the kernel with practical examples of spinlocks, mutexes, semaphores, and atomic operations.
5. [Interrupt Handling and Workqueues](./tutorial-05/) - Learn how to handle hardware interrupts and efficiently defer work using the kernel's workqueue system.

## Requirements

- Linux system (Raspberry Pi 5 recommended)
- Linux kernel headers (6.12+)
- Build essentials (gcc, make)

## Quick Start

The easiest way to build and test all modules is to use the provided scripts:

### Building All Modules

```bash
./build_all.sh
```

This will build all kernel modules without loading them into the kernel.

### Testing All Modules

```bash
sudo ./test_all.sh
```

This will build, load, and test each module automatically. The script requires root privileges to load the modules. It includes automatic cleanup to ensure all modules are unloaded even if an error occurs.

See [TESTING.md](./TESTING.md) for more details on the testing process.

## Building Modules Manually

Each tutorial directory contains a Makefile. To build a module, navigate to the specific tutorial directory and run:

```bash
make
```

## Testing Modules Manually

Each tutorial includes a test program (`test_*.c`) to demonstrate and test the functionality of the kernel module. To build and run the test program:

```bash
gcc -o test_program test_*.c
./test_program
```

See the README.md in each tutorial directory for specific instructions on using the test programs.

## Loading and Testing Modules Manually

After building, you can load a module using:

```bash
sudo insmod <module_name>.ko
```

And unload it with:

```bash
sudo rmmod <module_name>
```

Check the kernel logs to see the module's output:

```bash
dmesg | tail
```

## Compatibility

All modules have been updated to be compatible with Linux kernel 6.12 and higher, incorporating the latest kernel API changes.

## License

GPL v2 