#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/sync_demo"
#define PROC_PATH "/proc/sync_demo"
#define BUFFER_SIZE 1024

void display_device_info()
{
	int fd;
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	/* Open the device */
	fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device %s: %s\n", DEVICE_PATH,
			strerror(errno));
		return;
	}

	/* Read from the device */
	bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
	if (bytes_read < 0) {
		fprintf(stderr, "Failed to read from device: %s\n",
			strerror(errno));
		close(fd);
		return;
	}

	/* Null-terminate the buffer */
	buffer[bytes_read] = '\0';

	/* Display the device information */
	printf("\n==== Device Output ====\n");
	printf("%s\n", buffer);

	/* Close the device */
	close(fd);
}

void display_proc_info()
{
	FILE *fp;
	char buffer[BUFFER_SIZE];

	/* Open the proc file */
	fp = fopen(PROC_PATH, "r");
	if (!fp) {
		fprintf(stderr, "Failed to open proc file %s: %s\n", PROC_PATH,
			strerror(errno));
		return;
	}

	/* Display the proc information */
	printf("\n==== Proc Output ====\n");
	while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
		printf("%s", buffer);
	}

	/* Close the proc file */
	fclose(fp);
}

void reset_counters()
{
	int fd;
	const char *reset_cmd = "reset";

	/* Open the device for writing */
	fd = open(DEVICE_PATH, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device %s for writing: %s\n",
			DEVICE_PATH, strerror(errno));
		return;
	}

	/* Write the reset command */
	if (write(fd, reset_cmd, strlen(reset_cmd)) < 0) {
		fprintf(stderr, "Failed to write to device: %s\n",
			strerror(errno));
	} else {
		printf("\n==== Counters Reset ====\n");
	}

	/* Close the device */
	close(fd);
}

int main()
{
	printf("Sync Demo Test Program\n");
	printf("======================\n");

	/* Display initial device and proc information */
	display_device_info();
	display_proc_info();

	/* Reset the counters */
	reset_counters();

	/* Wait a moment for the kernel thread to update counters */
	printf("\nWaiting for 3 seconds to allow counters to update...\n");
	sleep(3);

	/* Display updated information */
	display_device_info();
	display_proc_info();

	/* Manual stress test options */
	char choice;
	printf("\nDo you want to run a stress test on the counters? (y/n): ");
	scanf(" %c", &choice);

	if (choice == 'y' || choice == 'Y') {
		int iterations;
		printf("Enter number of iterations (reset/read cycles): ");
		scanf("%d", &iterations);

		printf("\nRunning stress test with %d iterations...\n",
		       iterations);
		for (int i = 0; i < iterations; i++) {
			reset_counters();
			usleep(10000); /* 10ms */
			display_device_info();
			printf("Completed iteration %d/%d\n", i + 1,
			       iterations);
		}
		printf("\nStress test completed.\n");
	}

	return 0;
}