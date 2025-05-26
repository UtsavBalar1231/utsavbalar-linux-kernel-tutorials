# Tutorial 3: Kernel Memory Management for Drivers

This directory contains the source code for a kernel memory management demonstration module as featured in [Tutorial 3: Kernel Memory Management for Drivers](https://utsavbalar.com/tutorials/tutorial-03-kernel-memory-management-for-drivers).

## Files

- `kmem_demo.c` - Source code demonstrating various kernel memory allocation techniques
- `Makefile` - Build instructions for the module

## What This Module Demonstrates

This module demonstrates four primary kernel memory allocation mechanisms:

1. **kmalloc** - For small, physically contiguous memory blocks (4KB in this example)
2. **vmalloc** - For larger, virtually contiguous memory blocks (8MB in this example)
3. **__get_free_pages** - For page-level allocations (4 pages in this example)
4. **kmem_cache** - For efficient allocation of same-sized objects (custom struct in this example)

The module provides a proc file interface at `/proc/kmem_demo` to view the allocated memory details.

## Building the Module

To build the module, run:

```bash
make
```

This will compile the module and create `kmem_demo.ko`.

## Loading the Module

To load the module into the kernel:

```bash
sudo insmod kmem_demo.ko
```

Check that it loaded successfully:

```bash
lsmod | grep kmem_demo
```

View the kernel log messages to see the allocation results:

```bash
dmesg | tail
```

## Viewing Memory Information

After loading the module, you can view the memory allocation information:

```bash
cat /proc/kmem_demo
```

This will show details about all the different types of memory allocated by the module, including:
- Memory addresses
- Allocation sizes
- Memory flags used
- Cache information

## Unloading the Module

To unload the module:

```bash
sudo rmmod kmem_demo
```

Check the kernel log again to verify proper memory cleanup:

```bash
dmesg | tail
```

## Code Explanation

- The module demonstrates proper memory allocation techniques in the kernel
- Each allocation type is properly initialized and cleaned up
- Error handling with goto labels shows proper kernel coding patterns
- The module uses procfs to expose memory information to userspace
- Modern seq_file interface is used for proper procfs implementation

## Memory Management Best Practices

This example demonstrates several best practices for kernel memory management:

- Always check allocation return values
- Always free memory in the reverse order of allocation
- Use the appropriate allocation technique for each need
- Handle allocation failures gracefully
- Clean up all resources on module exit 