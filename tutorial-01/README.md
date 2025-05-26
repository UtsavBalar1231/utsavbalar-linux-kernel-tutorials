# Tutorial 1: Introduction to Linux Kernel Modules

This directory contains the source code for a simple "Hello World" Linux kernel module as demonstrated in [Tutorial 1: Introduction to Linux Kernel Modules](https://utsavbalar.com/tutorials/tutorial-01-introduction-to-linux-kernel-modules).

## Files

- `hello.c` - The source code for our kernel module
- `Makefile` - Build instructions for the module

## Building the Module

To build the module, simply run:

```bash
make
```

This will compile the module and create `hello.ko`.

## Loading the Module

To load the module into the kernel:

```bash
sudo insmod hello.ko
```

Check that it loaded successfully:

```bash
lsmod | grep hello
```

View the kernel log message:

```bash
dmesg | tail
```

You should see the message: `Hello World: Module loaded`

## Unloading the Module

To unload the module:

```bash
sudo rmmod hello
```

Check the kernel log again to see the unload message:

```bash
dmesg | tail
```

You should see: `Hello World: Module unloaded`

## Code Explanation

- The module uses standard kernel headers for module functionality
- `MODULE_*` macros provide metadata about the module
- `hello_init()` runs when the module is loaded (marked with `__init`)
- `hello_exit()` runs when the module is unloaded (marked with `__exit`)
- `module_init()` and `module_exit()` register these functions with the kernel
- `printk()` is used for kernel-space logging (similar to `printf()` in user space) 