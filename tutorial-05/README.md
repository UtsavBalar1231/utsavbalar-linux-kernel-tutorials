# Tutorial 5: Interrupt Handling and Workqueues

This directory contains the source code for an interrupt handling and workqueue demonstration module as featured in [Tutorial 5: Interrupt Handling and Workqueues](https://utsavbalar.com/tutorials/tutorial-05-interrupt-handling-and-workqueues).

## Files

- `irq_demo.c` - Source code demonstrating interrupt handling and workqueues in the kernel
- `Makefile` - Build instructions for the module
- `test_irq.c` - User-space test program for interacting with the module

## What This Module Demonstrates

This module demonstrates key interrupt handling and deferred work mechanisms in the Linux kernel:

1. **Top Half (IRQ Handler)** - The immediate interrupt handler that runs in interrupt context
2. **Bottom Half (Workqueue)** - Deferred work that runs in process context
3. **Delayed Work** - Work scheduled to run at a later time
4. **High-Resolution Timers** - Used to simulate hardware interrupts
5. **GPIO Interrupts** - (Optionally) Uses a GPIO pin to trigger real hardware interrupts

The module also tracks latency statistics between the top half and bottom half processing.

## Building the Module

To build the module, run:

```bash
make
```

This will compile the module and create `irq_demo.ko`.

## Loading the Module

To load the module into the kernel:

```bash
sudo insmod irq_demo.ko
```

Check that it loaded successfully:

```bash
lsmod | grep irq_demo
```

Check that the device node and proc entry were created:

```bash
ls -l /dev/irq_demo
ls -l /proc/irq_demo
```

View the kernel log messages:

```bash
dmesg | tail
```

## Viewing Interrupt Information

After loading the module, you can view the interrupt statistics:

```bash
cat /proc/irq_demo
```

This will show information about:
- Top-half (IRQ handler) execution count
- Bottom-half (workqueue) execution count
- Delayed work execution count
- Timing statistics and latency measurements

You can also read the device to get similar information:

```bash
cat /dev/irq_demo
```

## Testing Interrupt Handling

You can manually trigger the interrupt handler by writing "trigger" to the device:

```bash
echo "trigger" > /dev/irq_demo
```

You can reset all counters by writing "reset" to the device:

```bash
echo "reset" > /dev/irq_demo
```

## Building and Running the Test Program

To build the test program:

```bash
gcc -o test_irq test_irq.c
```

To run the test program:

```bash
./test_irq
```

This will:
1. Open the `/dev/irq_demo` device
2. Read and display the interrupt statistics
3. Trigger a manual interrupt
4. Reset the counters
5. Display updated statistics

## Hardware Setup (Optional)

If you want to use real GPIO interrupts (requires modifying the module to set `using_gpio = true`):

1. Connect a button to GPIO 17 (configurable in the code) and ground
2. Add a pull-up resistor (10kΩ) between GPIO 17 and 3.3V
3. Press the button to generate real hardware interrupts

## Unloading the Module

To unload the module:

```bash
sudo rmmod irq_demo
```

Check the kernel log to verify proper cleanup:

```bash
dmesg | tail
```

## Code Explanation

- The module demonstrates the complete interrupt handling flow in the kernel
- Shows proper synchronization between interrupt context and process context
- Implements both top-half (immediate) and bottom-half (deferred) processing
- Uses workqueues for bottom-half processing in process context
- Demonstrates delayed work scheduling
- Uses high-resolution timers for precise timing
- Properly implements procfs and character device interfaces
- Provides comprehensive statistics and latency measurements

## Best Practices Demonstrated

- Keep interrupt handlers (top half) short and fast
- Defer time-consuming work to the bottom half
- Use proper synchronization for shared data
- Use spinlocks in interrupt context
- Use workqueues for bottom-half processing that might sleep
- Properly clean up resources on module exit 