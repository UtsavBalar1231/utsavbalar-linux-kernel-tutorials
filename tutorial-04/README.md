# Tutorial 4: Synchronization Primitives

This directory contains the source code for a synchronization primitives demonstration module as featured in [Tutorial 4: Synchronization Primitives](https://utsavbalar.com/tutorials/tutorial-04-synchronization-primitives).

## Files

- `sync_demo.c` - Source code demonstrating various synchronization mechanisms in the kernel
- `Makefile` - Build instructions for the module
- `test_sync.c` - User-space test program for interacting with the module

## What This Module Demonstrates

This module demonstrates five key synchronization mechanisms in the Linux kernel:

1. **Atomic Operations** - For lockless counter updates
2. **Spinlocks** - For short-duration locks (busy-waiting)
3. **Mutexes** - For longer-duration locks (sleeping)
4. **Semaphores** - For controlling access to limited resources
5. **Read-Write Semaphores** - For allowing multiple readers or single writer

Each mechanism has a counter that is incremented in a kernel thread, demonstrating how they protect shared data from concurrent access.

## Building the Module

To build the module, run:

```bash
make
```

This will compile the module and create `sync_demo.ko`.

## Loading the Module

To load the module into the kernel:

```bash
sudo insmod sync_demo.ko
```

Check that it loaded successfully:

```bash
lsmod | grep sync_demo
```

Check that the device node and proc entry were created:

```bash
ls -l /dev/sync_demo
ls -l /proc/sync_demo
```

View the kernel log messages:

```bash
dmesg | tail
```

## Viewing Synchronization Information

After loading the module, you can view the synchronization counters:

```bash
cat /proc/sync_demo
```

This will show information about all counters, including:
- Atomic counter
- Spinlock-protected counter
- Mutex-protected counter
- Semaphore-protected counter
- Read-Write semaphore-protected counter

You can also read the device to get similar information:

```bash
cat /dev/sync_demo
```

## Testing Synchronization

You can reset all counters by writing "reset" to the device:

```bash
echo "reset" > /dev/sync_demo
```

## Building and Running the Test Program

To build the test program:

```bash
gcc -o test_sync test_sync.c
```

To run the test program:

```bash
./test_sync
```

This will:
1. Open the `/dev/sync_demo` device
2. Read and display the counter values
3. Reset the counters
4. Read and display the values again

## Unloading the Module

To unload the module:

```bash
sudo rmmod sync_demo
```

Check the kernel log to verify proper cleanup:

```bash
dmesg | tail
```

## Code Explanation

- The module creates a kernel thread that increments counters using different synchronization mechanisms
- Each synchronization primitive is properly initialized and used according to best practices
- The module implements both a character device and a proc file interface
- The code demonstrates proper locking patterns for various contexts
- Shows the performance and usage differences between various synchronization mechanisms

## Best Practices Demonstrated

- Use atomic operations for simple counters
- Use spinlocks for very short critical sections
- Use mutexes for longer operations that might sleep
- Use semaphores for resource limiting
- Use read-write mechanisms when reads are more common than writes 