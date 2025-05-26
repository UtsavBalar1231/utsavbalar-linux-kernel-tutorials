#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define DEVICE_PATH "/dev/irq_demo"
#define PROC_PATH "/proc/irq_demo"
#define BUFFER_SIZE 1024

void display_device_info() {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    /* Open the device */
    fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device %s: %s\n", DEVICE_PATH, strerror(errno));
        return;
    }

    /* Read from the device */
    bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        fprintf(stderr, "Failed to read from device: %s\n", strerror(errno));
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

void display_proc_info() {
    FILE *fp;
    char buffer[BUFFER_SIZE];

    /* Open the proc file */
    fp = fopen(PROC_PATH, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open proc file %s: %s\n", PROC_PATH, strerror(errno));
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

void trigger_interrupt() {
    int fd;
    const char *trigger_cmd = "trigger";

    /* Open the device for writing */
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device %s for writing: %s\n", DEVICE_PATH, strerror(errno));
        return;
    }

    /* Write the trigger command */
    if (write(fd, trigger_cmd, strlen(trigger_cmd)) < 0) {
        fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
    } else {
        printf("\n==== Interrupt Triggered ====\n");
    }

    /* Close the device */
    close(fd);
}

void reset_counters() {
    int fd;
    const char *reset_cmd = "reset";

    /* Open the device for writing */
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open device %s for writing: %s\n", DEVICE_PATH, strerror(errno));
        return;
    }

    /* Write the reset command */
    if (write(fd, reset_cmd, strlen(reset_cmd)) < 0) {
        fprintf(stderr, "Failed to write to device: %s\n", strerror(errno));
    } else {
        printf("\n==== Counters Reset ====\n");
    }

    /* Close the device */
    close(fd);
}

void run_latency_test(int iterations) {
    printf("\n==== Running Latency Test (%d iterations) ====\n", iterations);
    
    /* Reset the counters to start fresh */
    reset_counters();
    
    /* Trigger interrupts and measure latency */
    for (int i = 0; i < iterations; i++) {
        trigger_interrupt();
        usleep(100000); /* 100ms - allow time for bottom half to complete */
        
        if (i == 0 || i == iterations - 1 || (iterations > 10 && i % (iterations / 10) == 0)) {
            /* Display stats at first, last, and every 10% if iterations > 10 */
            display_proc_info();
        }
        
        printf("Completed iteration %d/%d\n", i+1, iterations);
    }
}

int main(int argc, char *argv[]) {
    printf("IRQ Demo Test Program\n");
    printf("====================\n");

    /* Display initial device and proc information */
    display_device_info();
    display_proc_info();

    /* Basic interrupt test */
    printf("\nPerforming basic interrupt test...\n");
    
    /* Trigger a manual interrupt */
    trigger_interrupt();
    
    /* Wait a moment for the bottom half to execute */
    printf("Waiting for bottom half to execute...\n");
    sleep(1);
    
    /* Display updated information */
    display_device_info();
    display_proc_info();
    
    /* Reset the counters */
    reset_counters();
    
    /* Display reset information */
    display_device_info();
    
    /* Interactive menu */
    int choice = 0;
    int iterations;
    
    while (choice != 5) {
        printf("\n==== IRQ Demo Test Menu ====\n");
        printf("1. Display current statistics\n");
        printf("2. Trigger a manual interrupt\n");
        printf("3. Reset all counters\n");
        printf("4. Run latency test\n");
        printf("5. Exit\n");
        printf("Enter your choice (1-5): ");
        
        if (scanf("%d", &choice) != 1) {
            /* Clear input buffer on invalid input */
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            choice = 0;
            continue;
        }
        
        switch (choice) {
            case 1:
                display_device_info();
                display_proc_info();
                break;
            case 2:
                trigger_interrupt();
                sleep(1); /* Give time for bottom half */
                break;
            case 3:
                reset_counters();
                break;
            case 4:
                printf("Enter number of iterations for latency test: ");
                if (scanf("%d", &iterations) == 1 && iterations > 0) {
                    run_latency_test(iterations);
                } else {
                    printf("Invalid number of iterations\n");
                }
                break;
            case 5:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice. Please enter a number between 1 and 5.\n");
                break;
        }
    }

    return 0;
} 