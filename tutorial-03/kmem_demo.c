#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>         /* For kmalloc, kfree */
#include <linux/vmalloc.h>      /* For vmalloc, vfree */
#include <linux/mm.h>           /* For get_free_pages */
#include <linux/uaccess.h>      /* For copy_to_user, copy_from_user */

#define PROCFS_NAME "kmem_demo"
#define KMALLOC_SIZE (4 * 1024)        /* 4 KB */
#define VMALLOC_SIZE (8 * 1024 * 1024) /* 8 MB */
#define PAGE_ORDER 2                    /* 2^2 = 4 pages */

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Utsav Balar");
MODULE_DESCRIPTION("Kernel memory management demonstration module");
MODULE_VERSION("0.1");

/* Memory pointers for different allocation types */
static void *kmalloc_ptr = NULL;
static void *vmalloc_ptr = NULL;
static unsigned long page_ptr = 0;
static struct kmem_cache *cache = NULL;
static void *cache_ptr = NULL;

/* Structure for kmem_cache example */
struct demo_struct {
    int id;
    char name[32];
    struct list_head list;
};

/* Initialize memory allocations */
static int __init init_memory(void)
{
    /* 1. kmalloc example - 4KB with GFP_KERNEL */
    kmalloc_ptr = kmalloc(KMALLOC_SIZE, GFP_KERNEL);
    if (!kmalloc_ptr) {
        pr_err("kmem_demo: Failed to allocate kmalloc memory\n");
        goto fail_kmalloc;
    }
    memset(kmalloc_ptr, 0, KMALLOC_SIZE);
    pr_info("kmem_demo: Allocated %d bytes with kmalloc at address 0x%px\n", 
           KMALLOC_SIZE, kmalloc_ptr);
    
    /* 2. vmalloc example - 8MB */
    vmalloc_ptr = vmalloc(VMALLOC_SIZE);
    if (!vmalloc_ptr) {
        pr_err("kmem_demo: Failed to allocate vmalloc memory\n");
        goto fail_vmalloc;
    }
    memset(vmalloc_ptr, 0, VMALLOC_SIZE);
    pr_info("kmem_demo: Allocated %d bytes with vmalloc at address 0x%px\n", 
           VMALLOC_SIZE, vmalloc_ptr);
    
    /* 3. get_free_pages example - 4 pages = 16KB on systems with 4KB pages */
    page_ptr = __get_free_pages(GFP_KERNEL, PAGE_ORDER);
    if (!page_ptr) {
        pr_err("kmem_demo: Failed to allocate pages\n");
        goto fail_pages;
    }
    memset((void *)page_ptr, 0, PAGE_SIZE << PAGE_ORDER);
    pr_info("kmem_demo: Allocated %lu bytes with get_free_pages at address 0x%lx\n", 
           PAGE_SIZE << PAGE_ORDER, page_ptr);
    
    /* 4. kmem_cache example - custom object cache */
    cache = kmem_cache_create("demo_cache", sizeof(struct demo_struct), 
                             0, SLAB_HWCACHE_ALIGN, NULL);
    if (!cache) {
        pr_err("kmem_demo: Failed to create kmem_cache\n");
        goto fail_cache_create;
    }
    
    /* Allocate an object from the cache */
    cache_ptr = kmem_cache_alloc(cache, GFP_KERNEL);
    if (!cache_ptr) {
        pr_err("kmem_demo: Failed to allocate from kmem_cache\n");
        goto fail_cache_alloc;
    }
    
    /* Initialize the allocated object */
    memset(cache_ptr, 0, sizeof(struct demo_struct));
    ((struct demo_struct *)cache_ptr)->id = 1;
    strcpy(((struct demo_struct *)cache_ptr)->name, "Cache Example");
    
    pr_info("kmem_demo: Allocated object of size %lu bytes from kmem_cache at address 0x%px\n", 
           sizeof(struct demo_struct), cache_ptr);
    
    return 0;

/* Error handling and cleanup */
fail_cache_alloc:
    kmem_cache_destroy(cache);
fail_cache_create:
    free_pages(page_ptr, PAGE_ORDER);
fail_pages:
    vfree(vmalloc_ptr);
fail_vmalloc:
    kfree(kmalloc_ptr);
fail_kmalloc:
    return -ENOMEM;
}

/* Release allocated memory */
static void free_memory(void)
{
    /* Free all allocated memory in reverse order of allocation */
    if (cache_ptr)
        kmem_cache_free(cache, cache_ptr);
    
    if (cache)
        kmem_cache_destroy(cache);
    
    if (page_ptr)
        free_pages(page_ptr, PAGE_ORDER);
    
    if (vmalloc_ptr)
        vfree(vmalloc_ptr);
    
    if (kmalloc_ptr)
        kfree(kmalloc_ptr);
    
    pr_info("kmem_demo: All memory freed\n");
}

/* ProcFS handlers for displaying memory information */
static int kmem_demo_show(struct seq_file *m, void *v)
{
    seq_puts(m, "Kernel Memory Management Demo Module\n");
    seq_puts(m, "==================================\n\n");
    
    /* Show kmalloc information */
    seq_printf(m, "1. kmalloc:\n");
    seq_printf(m, "   Size: %d bytes\n", KMALLOC_SIZE);
    seq_printf(m, "   Address: 0x%px\n", kmalloc_ptr);
    seq_printf(m, "   Flags used: GFP_KERNEL\n\n");
    
    /* Show vmalloc information */
    seq_printf(m, "2. vmalloc:\n");
    seq_printf(m, "   Size: %d bytes\n", VMALLOC_SIZE);
    seq_printf(m, "   Address: 0x%px\n\n", vmalloc_ptr);
    
    /* Show get_free_pages information */
    seq_printf(m, "3. __get_free_pages:\n");
    seq_printf(m, "   Order: %d (2^%d pages)\n", PAGE_ORDER, PAGE_ORDER);
    seq_printf(m, "   Size: %lu bytes\n", PAGE_SIZE << PAGE_ORDER);
    seq_printf(m, "   Address: 0x%lx\n\n", page_ptr);
    
    /* Show kmem_cache information */
    seq_printf(m, "4. kmem_cache:\n");
    seq_printf(m, "   Object size: %lu bytes\n", sizeof(struct demo_struct));
    seq_printf(m, "   Cache name: %s\n", cache ? "demo_cache" : "N/A");
    if (cache_ptr) {
        struct demo_struct *obj = (struct demo_struct *)cache_ptr;
        seq_printf(m, "   Object address: 0x%px\n", cache_ptr);
        seq_printf(m, "   Object id: %d\n", obj->id);
        seq_printf(m, "   Object name: %s\n", obj->name);
    }
    
    return 0;
}

static int kmem_demo_open(struct inode *inode, struct file *file)
{
    return single_open(file, kmem_demo_show, NULL);
}

static const struct proc_ops kmem_demo_fops = {
    .proc_open = kmem_demo_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* Module initialization function */
static int __init kmem_demo_init(void)
{
    struct proc_dir_entry *proc_file;
    int ret;
    
    /* Initialize memory allocations */
    ret = init_memory();
    if (ret)
        return ret;
    
    /* Create proc file */
    proc_file = proc_create(PROCFS_NAME, 0444, NULL, &kmem_demo_fops);
    if (!proc_file) {
        pr_err("kmem_demo: Failed to create proc entry\n");
        free_memory();
        return -ENOMEM;
    }
    
    pr_info("kmem_demo: Module loaded\n");
    pr_info("kmem_demo: Created proc entry /proc/%s\n", PROCFS_NAME);
    return 0;
}

/* Module cleanup function */
static void __exit kmem_demo_exit(void)
{
    /* Remove proc file */
    remove_proc_entry(PROCFS_NAME, NULL);
    
    /* Free all memory */
    free_memory();
    
    pr_info("kmem_demo: Module unloaded\n");
}

/* Register module init/exit functions */
module_init(kmem_demo_init);
module_exit(kmem_demo_exit); 