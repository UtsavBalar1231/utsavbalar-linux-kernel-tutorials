# Tutorial 2: Character Device Drivers Fundamentals

This directory contains the source code for a simple character device driver as demonstrated in [Tutorial 2: Character Device Drivers Fundamentals](https://utsavbalar.com/tutorials/tutorial-02-character-device-drivers-fundamentals).

## Files

- `simple_char.c` - Source code for the character device driver
- `Makefile` - Build instructions for the module

## What This Driver Does

This module creates a character device at `/dev/simple_char` that acts as a 1KB memory buffer:

- Reading from the device returns data from the buffer
- Writing to the device stores data in the buffer
- The driver supports file positioning with `lseek()`
- All operations are properly registered through the file_operations structure

## Building the Module

To build the module, run:

```bash
make
```

This will compile the module and create `simple_char.ko`.

## Loading the Module

To load the module into the kernel:

```bash
sudo insmod simple_char.ko
```

Check that it loaded successfully:

```bash
lsmod | grep simple_char
```

Check that the device node was created:

```bash
ls -l /dev/simple_char
```

View the kernel log messages:

```bash
dmesg | tail
```

## Testing the Device

You can interact with the device using standard file operations:

### Writing to the device:

```bash
echo "Hello, Character Device!" > /dev/simple_char
```

### Reading from the device:

```bash
cat /dev/simple_char
```

### Testing with seek:

```bash
# To test seeking and partial reads/writes
dd if=/dev/urandom of=/dev/simple_char bs=512 count=1
dd if=/dev/simple_char of=/dev/null bs=64 count=1 skip=3
```

## Unloading the Module

To unload the module:

```bash
sudo rmmod simple_char
```

Check the kernel log again to see the cleanup messages:

```bash
dmesg | tail
```

## Code Explanation

- The module implements the core file operations: open, release, read, write, and llseek
- It uses modern kernel interfaces like device_create() and class_create()
- Copy_to_user() and copy_from_user() ensure safe data transfer between kernel and user space
- The cdev interface is used for modern character device registration
- Proper error handling and cleanup is implemented to prevent resource leaks 