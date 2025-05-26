# Linux Kernel Module Tutorials

This repository contains practical examples of Linux kernel modules corresponding to the tutorial series on [Utsav Balar's website](https://utsavbalar.com/tutorials).

## Tutorials

1. [Introduction to Linux Kernel Modules](./tutorial-01/) - Learn the fundamentals of Linux kernel modules and create your first 'Hello World' kernel module.
2. [Character Device Drivers Fundamentals](./tutorial-02/) - Master the essentials of character device drivers with a simple memory buffer device.
3. [Kernel Memory Management for Drivers](./tutorial-03/) - Explore various memory allocation techniques in the kernel with practical examples.

## Requirements

- Linux system (Raspberry Pi 5 recommended)
- Linux kernel headers
- Build essentials (gcc, make)

## Building Modules

Each tutorial directory contains a Makefile. To build a module, navigate to the specific tutorial directory and run:

```bash
make
```

## Loading and Testing Modules

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

## License

GPL v2 