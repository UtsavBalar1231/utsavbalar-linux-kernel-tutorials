#include <linux/module.h> // Core module functionality
#include <linux/kernel.h> // For KERN_INFO
#include <linux/init.h> // For module_init and module_exit macros

MODULE_LICENSE("GPL"); // Module license
MODULE_AUTHOR("Utsav Balar"); // Module author
MODULE_DESCRIPTION("A simple Hello World kernel module"); // Module description
MODULE_VERSION("0.1"); // Module version

/**
 * hello_init - Module initialization function
 *
 * This function runs when the module is loaded into the kernel.
 * It prints a welcome message to the kernel log.
 *
 * Return: 0 on success, negative errno on failure
 */
static int __init hello_init(void)
{
	printk(KERN_INFO "Hello World: Module loaded\n");
	return 0;
}

/**
 * hello_exit - Module cleanup function
 *
 * This function runs when the module is unloaded from the kernel.
 * It prints a goodbye message to the kernel log.
 */
static void __exit hello_exit(void)
{
	printk(KERN_INFO "Hello World: Module unloaded\n");
}

// Register module initialization and cleanup functions
module_init(hello_init);
module_exit(hello_exit);