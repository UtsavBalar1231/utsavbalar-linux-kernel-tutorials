#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/simple_char"
#define BUFFER_SIZE 1024

void display_usage(const char *program_name)
{
	printf("Usage: %s [command] [options]\n", program_name);
	printf("Commands:\n");
	printf("  read [offset] [length]   - Read from device (default: offset=0, length=all)\n");
	printf("  write <data>             - Write data to device\n");
	printf("  test                     - Run a comprehensive test suite\n");
	printf("  load                     - Load the module\n");
	printf("  unload                   - Unload the module\n");
	printf("  help                     - Display this help message\n");
}

int load_module()
{
	int ret;

	/* Check if module is already loaded */
	ret = system("lsmod | grep simple_char > /dev/null");
	if (ret == 0) {
		printf("Module 'simple_char' is already loaded. Unloading first...\n");
		ret = system("sudo rmmod simple_char");
		if (ret != 0) {
			fprintf(stderr, "Error: Failed to unload module\n");
			return 1;
		}
		printf("Module unloaded successfully.\n");
	}

	/* Check if the module is built */
	if (access("simple_char.ko", F_OK) != 0) {
		printf("Module 'simple_char.ko' not found. Building...\n");
		ret = system("make");
		if (ret != 0) {
			fprintf(stderr, "Error: Failed to build module\n");
			return 1;
		}
		printf("Module built successfully.\n");
	}

	/* Load the module */
	printf("Loading module...\n");
	ret = system("sudo insmod simple_char.ko");
	if (ret != 0) {
		fprintf(stderr, "Error: Failed to load module\n");
		return 1;
	}
	printf("Module loaded successfully.\n");

	/* Check device node exists */
	if (access(DEVICE_PATH, F_OK) != 0) {
		fprintf(stderr, "Error: Device node %s does not exist\n",
			DEVICE_PATH);
		return 1;
	}

	return 0;
}

int unload_module()
{
	int ret;

	/* Check if module is loaded */
	ret = system("lsmod | grep simple_char > /dev/null");
	if (ret != 0) {
		fprintf(stderr, "Module 'simple_char' is not loaded\n");
		return 1;
	}

	/* Unload the module */
	printf("Unloading module...\n");
	ret = system("sudo rmmod simple_char");
	if (ret != 0) {
		fprintf(stderr, "Error: Failed to unload module\n");
		return 1;
	}
	printf("Module unloaded successfully.\n");

	return 0;
}

int read_device(int offset, int length)
{
	int fd;
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	/* Open the device */
	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device %s: %s\n", DEVICE_PATH,
			strerror(errno));
		return 1;
	}

	/* Seek to the specified offset */
	if (offset > 0) {
		if (lseek(fd, offset, SEEK_SET) < 0) {
			fprintf(stderr, "Failed to seek to offset %d: %s\n",
				offset, strerror(errno));
			close(fd);
			return 1;
		}
		printf("Seeked to offset %d\n", offset);
	}

	/* Read from the device */
	if (length <= 0 || length > BUFFER_SIZE - 1) {
		length = BUFFER_SIZE - 1;
	}

	bytes_read = read(fd, buffer, length);
	if (bytes_read < 0) {
		fprintf(stderr, "Failed to read from device: %s\n",
			strerror(errno));
		close(fd);
		return 1;
	}

	/* Null-terminate the buffer */
	buffer[bytes_read] = '\0';

	/* Display the read data */
	printf("\n=== Read %zd bytes from offset %d ===\n", bytes_read, offset);

	/* Display as string if mostly printable */
	int printable_chars = 0;
	for (int i = 0; i < bytes_read; i++) {
		if (buffer[i] >= 32 && buffer[i] <= 126) {
			printable_chars++;
		}
	}

	if (printable_chars > bytes_read * 0.8) {
		/* Mostly printable, display as string */
		printf("%s\n", buffer);
	} else {
		/* Not mostly printable, display as hex dump */
		printf("Hex dump:\n");
		for (int i = 0; i < bytes_read; i++) {
			printf("%02x ", (unsigned char)buffer[i]);
			if ((i + 1) % 16 == 0) {
				printf("\n");
			}
		}
		printf("\n");
	}

	/* Close the device */
	close(fd);
	return 0;
}

int write_device(const char *data)
{
	int fd;
	ssize_t bytes_written;

	/* Open the device */
	fd = open(DEVICE_PATH, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device %s for writing: %s\n",
			DEVICE_PATH, strerror(errno));
		return 1;
	}

	/* Write to the device */
	bytes_written = write(fd, data, strlen(data));
	if (bytes_written < 0) {
		fprintf(stderr, "Failed to write to device: %s\n",
			strerror(errno));
		close(fd);
		return 1;
	}

	/* Display the result */
	printf("Successfully wrote %zd bytes to device\n", bytes_written);

	/* Close the device */
	close(fd);
	return 0;
}

int run_tests()
{
	int ret;

	printf("\n=== Running Character Device Driver Tests ===\n\n");

	/* Load the module if not already loaded */
	ret = system("lsmod | grep simple_char > /dev/null");
	if (ret != 0) {
		if (load_module() != 0) {
			return 1;
		}
	}

	/* Test 1: Write a string */
	printf("Test 1: Writing a string to the device...\n");
	const char *test_string = "Hello, Character Device Driver!";
	ret = write_device(test_string);
	if (ret != 0) {
		return 1;
	}

	/* Test 2: Read the string back */
	printf("\nTest 2: Reading the string back...\n");
	ret = read_device(0, strlen(test_string));
	if (ret != 0) {
		return 1;
	}

	/* Test 3: Write to a specific offset */
	printf("\nTest 3: Writing to offset 10...\n");
	int fd = open(DEVICE_PATH, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
		return 1;
	}

	if (lseek(fd, 10, SEEK_SET) < 0) {
		fprintf(stderr, "Failed to seek: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	const char *offset_string = "OFFSET WRITE";
	ssize_t bytes_written = write(fd, offset_string, strlen(offset_string));
	close(fd);

	if (bytes_written < 0) {
		fprintf(stderr, "Failed to write at offset: %s\n",
			strerror(errno));
		return 1;
	}

	printf("Successfully wrote %zd bytes at offset 10\n", bytes_written);

	/* Test 4: Read the modified content */
	printf("\nTest 4: Reading the entire buffer after offset write...\n");
	ret = read_device(0, 50);
	if (ret != 0) {
		return 1;
	}

	/* Test 5: Read with an offset */
	printf("\nTest 5: Reading with offset 10...\n");
	ret = read_device(10, 20);
	if (ret != 0) {
		return 1;
	}

	/* Test 6: Fill the buffer with binary data */
	printf("\nTest 6: Writing binary data...\n");
	fd = open(DEVICE_PATH, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
		return 1;
	}

	unsigned char binary_data[50];
	for (int i = 0; i < 50; i++) {
		binary_data[i] = i;
	}

	bytes_written = write(fd, binary_data, 50);
	close(fd);

	if (bytes_written < 0) {
		fprintf(stderr, "Failed to write binary data: %s\n",
			strerror(errno));
		return 1;
	}

	printf("Successfully wrote %zd bytes of binary data\n", bytes_written);

	/* Test 7: Read the binary data */
	printf("\nTest 7: Reading binary data...\n");
	ret = read_device(0, 50);
	if (ret != 0) {
		return 1;
	}

	printf("\nAll tests completed successfully!\n");
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		display_usage(argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "read") == 0) {
		int offset = 0;
		int length = 0;

		if (argc >= 3) {
			offset = atoi(argv[2]);
		}

		if (argc >= 4) {
			length = atoi(argv[3]);
		}

		return read_device(offset, length);
	} else if (strcmp(argv[1], "write") == 0) {
		if (argc < 3) {
			fprintf(stderr, "Error: No data specified for write\n");
			display_usage(argv[0]);
			return 1;
		}

		return write_device(argv[2]);
	} else if (strcmp(argv[1], "test") == 0) {
		return run_tests();
	} else if (strcmp(argv[1], "load") == 0) {
		return load_module();
	} else if (strcmp(argv[1], "unload") == 0) {
		return unload_module();
	} else if (strcmp(argv[1], "help") == 0) {
		display_usage(argv[0]);
		return 0;
	} else {
		fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
		display_usage(argv[0]);
		return 1;
	}
}