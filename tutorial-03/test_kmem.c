#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define PROC_PATH "/proc/kmem_demo"
#define BUFFER_SIZE 2048

void display_usage(const char *program_name) {
    printf("Usage: %s [command]\n", program_name);
    printf("Commands:\n");
    printf("  display    - Display memory allocation information\n");
    printf("  load       - Load the module\n");
    printf("  unload     - Unload the module\n");
    printf("  monitor    - Monitor memory allocations over time\n");
    printf("  help       - Display this help message\n");
}

int load_module() {
    int ret;
    
    /* Check if module is already loaded */
    ret = system("lsmod | grep kmem_demo > /dev/null");
    if (ret == 0) {
        printf("Module 'kmem_demo' is already loaded. Unloading first...\n");
        ret = system("sudo rmmod kmem_demo");
        if (ret != 0) {
            fprintf(stderr, "Error: Failed to unload module\n");
            return 1;
        }
        printf("Module unloaded successfully.\n");
    }
    
    /* Check if the module is built */
    if (access("kmem_demo.ko", F_OK) != 0) {
        printf("Module 'kmem_demo.ko' not found. Building...\n");
        ret = system("make");
        if (ret != 0) {
            fprintf(stderr, "Error: Failed to build module\n");
            return 1;
        }
        printf("Module built successfully.\n");
    }
    
    /* Load the module */
    printf("Loading module...\n");
    ret = system("sudo insmod kmem_demo.ko");
    if (ret != 0) {
        fprintf(stderr, "Error: Failed to load module\n");
        return 1;
    }
    printf("Module loaded successfully.\n");
    
    /* Check proc file exists */
    if (access(PROC_PATH, F_OK) != 0) {
        fprintf(stderr, "Error: Proc file %s does not exist\n", PROC_PATH);
        return 1;
    }
    
    return 0;
}

int unload_module() {
    int ret;
    
    /* Check if module is loaded */
    ret = system("lsmod | grep kmem_demo > /dev/null");
    if (ret != 0) {
        fprintf(stderr, "Module 'kmem_demo' is not loaded\n");
        return 1;
    }
    
    /* Unload the module */
    printf("Unloading module...\n");
    ret = system("sudo rmmod kmem_demo");
    if (ret != 0) {
        fprintf(stderr, "Error: Failed to unload module\n");
        return 1;
    }
    printf("Module unloaded successfully.\n");
    
    return 0;
}

int display_memory_info() {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    
    /* Open the proc file */
    fp = fopen(PROC_PATH, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open proc file %s: %s\n", PROC_PATH, strerror(errno));
        fprintf(stderr, "Make sure the module is loaded with 'sudo insmod kmem_demo.ko'\n");
        return 1;
    }
    
    /* Read and display the memory information */
    printf("\n=== Kernel Memory Allocation Information ===\n\n");
    
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        printf("%s", buffer);
    }
    
    /* Close the proc file */
    fclose(fp);
    
    /* Show system memory info for context */
    printf("\n=== System Memory Information ===\n\n");
    system("free -m");
    
    printf("\n=== Kernel Memory Allocation Types ===\n\n");
    printf("1. kmalloc: For smaller allocations requiring physically contiguous memory\n");
    printf("2. vmalloc: For larger allocations requiring only virtually contiguous memory\n");
    printf("3. get_free_pages: For page-level allocations\n");
    printf("4. kmem_cache: For efficient allocation of same-sized objects\n");
    
    return 0;
}

int monitor_memory() {
    int ret;
    int interval = 2; /* seconds */
    int iterations = 10;
    
    printf("Monitoring memory allocations for %d iterations (%d second intervals)...\n",
           iterations, interval);
    
    /* Check if module is loaded */
    ret = system("lsmod | grep kmem_demo > /dev/null");
    if (ret != 0) {
        printf("Module not loaded. Loading now...\n");
        if (load_module() != 0) {
            return 1;
        }
    }
    
    /* Monitor memory allocations */
    for (int i = 0; i < iterations; i++) {
        /* Clear screen for better visibility */
        system("clear");
        
        printf("=== Memory Monitor - Iteration %d/%d ===\n\n", i+1, iterations);
        
        /* Display kernel memory info */
        display_memory_info();
        
        /* Display memory allocation addresses */
        printf("\n=== Kernel Memory Addresses ===\n");
        system("grep kmem_demo /proc/kallsyms | grep -v module_layout | head -10");
        
        /* Wait before next iteration */
        if (i < iterations - 1) {
            printf("\nWaiting %d seconds for next reading...\n", interval);
            sleep(interval);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        display_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "display") == 0) {
        return display_memory_info();
    }
    else if (strcmp(argv[1], "load") == 0) {
        return load_module();
    }
    else if (strcmp(argv[1], "unload") == 0) {
        return unload_module();
    }
    else if (strcmp(argv[1], "monitor") == 0) {
        return monitor_memory();
    }
    else if (strcmp(argv[1], "help") == 0) {
        display_usage(argv[0]);
        return 0;
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        display_usage(argv[0]);
        return 1;
    }
} 