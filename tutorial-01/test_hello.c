#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
	int ret;

	printf("Hello World Kernel Module Test\n");
	printf("=============================\n\n");

	/* Check if module is already loaded */
	ret = system("lsmod | grep hello > /dev/null");
	if (ret == 0) {
		printf("Module 'hello' is already loaded. Unloading first...\n");
		ret = system("sudo rmmod hello");
		if (ret != 0) {
			fprintf(stderr, "Error: Failed to unload module\n");
			return 1;
		}
		printf("Module unloaded successfully.\n");
	}

	/* Check if the module is built */
	if (access("hello.ko", F_OK) != 0) {
		printf("Module 'hello.ko' not found. Building...\n");
		ret = system("make");
		if (ret != 0) {
			fprintf(stderr, "Error: Failed to build module\n");
			return 1;
		}
		printf("Module built successfully.\n");
	}

	/* Load the module */
	printf("\nLoading module...\n");
	ret = system("sudo insmod hello.ko");
	if (ret != 0) {
		fprintf(stderr, "Error: Failed to load module\n");
		return 1;
	}
	printf("Module loaded successfully.\n");

	/* Check module is loaded */
	printf("\nVerifying module is loaded:\n");
	system("lsmod | grep hello");

	/* Check kernel log */
	printf("\nChecking kernel log messages:\n");
	system("sudo dmesg | tail -n 5");

	/* Wait a bit */
	printf("\nWaiting for 3 seconds...\n");
	sleep(3);

	/* Unload the module */
	printf("\nUnloading module...\n");
	ret = system("sudo rmmod hello");
	if (ret != 0) {
		fprintf(stderr, "Error: Failed to unload module\n");
		return 1;
	}
	printf("Module unloaded successfully.\n");

	/* Check kernel log again */
	printf("\nChecking kernel log messages after unload:\n");
	system("sudo dmesg | tail -n 5");

	printf("\nTest completed successfully!\n");
	return 0;
}
