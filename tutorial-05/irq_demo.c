#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>

#define DEVICE_NAME "irq_demo"
#define CLASS_NAME "irq"
#define BUFFER_SIZE 1024

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Utsav Balar");
MODULE_DESCRIPTION("Interrupt handling and workqueue demonstration module");
MODULE_VERSION("0.1");

/* GPIO pin for simulated interrupt (change as needed for your Raspberry Pi) */
#define BUTTON_GPIO 17  /* Using GPIO 17 as example, adjust for your hardware */

/* Timer period (in nanoseconds) for simulated interrupts */
#define TIMER_PERIOD_NS 1000000000  /* 1 second */

/* Workqueue and work variables */
static struct workqueue_struct *demo_wq;
static struct work_struct regular_work;
static struct delayed_work delayed_work;

/* Timer for simulating interrupts */
static struct hrtimer demo_timer;

/* Interrupt statistics */
static atomic_t irq_count = ATOMIC_INIT(0);
static atomic_t bottom_half_count = ATOMIC_INIT(0);
static atomic_t delayed_work_count = ATOMIC_INIT(0);
static ktime_t last_irq_time;
static ktime_t last_bh_time;
static spinlock_t stats_lock;
static struct mutex proc_mutex;

/* Device variables */
static int major_number;
static struct class *irq_class = NULL;
static struct device *irq_device = NULL;
static struct cdev irq_cdev;
static char device_buffer[BUFFER_SIZE];

/* IRQ number */
static int button_irq;
static bool using_gpio = false;
static bool using_timer = true;

/* Forward declarations */
static int irq_demo_open(struct inode *, struct file *);
static int irq_demo_release(struct inode *, struct file *);
static ssize_t irq_demo_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t irq_demo_write(struct file *, const char __user *, size_t, loff_t *);

/* File operations */
static struct file_operations irq_fops = {
    .owner = THIS_MODULE,
    .open = irq_demo_open,
    .release = irq_demo_release,
    .read = irq_demo_read,
    .write = irq_demo_write,
};

/* ProcFS handler */
static int irq_proc_show(struct seq_file *m, void *v)
{
    ktime_t irq_time, bh_time;
    unsigned long flags;
    
    /* Use mutex for proc access */
    mutex_lock(&proc_mutex);
    
    /* Get consistent view of timestamps */
    spin_lock_irqsave(&stats_lock, flags);
    irq_time = last_irq_time;
    bh_time = last_bh_time;
    spin_unlock_irqrestore(&stats_lock, flags);
    
    /* Output the statistics */
    seq_puts(m, "Interrupt Handling Demo Statistics\n");
    seq_puts(m, "==================================\n\n");
    
    seq_printf(m, "Top-half (IRQ) count: %d\n", atomic_read(&irq_count));
    seq_printf(m, "Bottom-half (work) count: %d\n", atomic_read(&bottom_half_count));
    seq_printf(m, "Delayed work count: %d\n", atomic_read(&delayed_work_count));
    
    if (using_gpio) {
        seq_printf(m, "Using GPIO %d for hardware interrupts\n", BUTTON_GPIO);
    }
    
    if (using_timer) {
        seq_printf(m, "Using timer to simulate interrupts (period: %lld ns)\n", TIMER_PERIOD_NS);
    }
    
    if (irq_time) {
        seq_printf(m, "Last IRQ time: %lld ns\n", ktime_to_ns(irq_time));
    }
    
    if (bh_time) {
        seq_printf(m, "Last bottom-half time: %lld ns\n", ktime_to_ns(bh_time));
        
        if (irq_time) {
            /* Calculate latency between IRQ and bottom-half */
            s64 latency_ns = ktime_to_ns(ktime_sub(bh_time, irq_time));
            seq_printf(m, "IRQ to bottom-half latency: %lld ns\n", latency_ns);
        }
    }
    
    mutex_unlock(&proc_mutex);
    return 0;
}

static int irq_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, irq_proc_show, NULL);
}

static const struct proc_ops irq_proc_fops = {
    .proc_open = irq_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* Regular workqueue function (bottom half) */
static void demo_work_handler(struct work_struct *work)
{
    unsigned long flags;
    
    /* Record timestamp */
    ktime_t now = ktime_get();
    
    /* Update statistics */
    atomic_inc(&bottom_half_count);
    
    /* Use spinlock to protect shared data (timestamps) */
    spin_lock_irqsave(&stats_lock, flags);
    last_bh_time = now;
    spin_unlock_irqrestore(&stats_lock, flags);
    
    pr_info("irq_demo: Bottom half (work) executed, count: %d\n", 
           atomic_read(&bottom_half_count));
}

/* Delayed workqueue function */
static void demo_delayed_work_handler(struct work_struct *work)
{
    /* Update counter */
    atomic_inc(&delayed_work_count);
    
    pr_info("irq_demo: Delayed work executed, count: %d\n", 
           atomic_read(&delayed_work_count));
    
    /* Reschedule the delayed work */
    queue_delayed_work(demo_wq, &delayed_work, msecs_to_jiffies(5000)); /* 5 seconds */
}

/* IRQ handler (top half) */
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    unsigned long flags;
    
    /* Record timestamp */
    ktime_t now = ktime_get();
    
    /* Update IRQ counter */
    atomic_inc(&irq_count);
    
    /* Use spinlock to protect shared data (timestamp) */
    spin_lock_irqsave(&stats_lock, flags);
    last_irq_time = now;
    spin_unlock_irqrestore(&stats_lock, flags);
    
    /* Schedule the bottom half */
    queue_work(demo_wq, &regular_work);
    
    pr_info("irq_demo: Interrupt handled, count: %d\n", atomic_read(&irq_count));
    
    return IRQ_HANDLED;
}

/* Timer callback to simulate interrupts */
static enum hrtimer_restart demo_timer_callback(struct hrtimer *timer)
{
    /* Call the IRQ handler directly */
    button_irq_handler(0, NULL);
    
    /* Reschedule the timer */
    hrtimer_forward_now(timer, ns_to_ktime(TIMER_PERIOD_NS));
    
    return HRTIMER_RESTART;
}

/* Character device functions */
static int irq_demo_open(struct inode *inode, struct file *file)
{
    /* Nothing special to do here */
    return 0;
}

static int irq_demo_release(struct inode *inode, struct file *file)
{
    /* Nothing special to do here */
    return 0;
}

static ssize_t irq_demo_read(struct file *file, char __user *user_buffer, 
                             size_t count, loff_t *offset)
{
    int bytes_to_read;
    int bytes_not_copied;
    int len;
    
    /* Prepare the buffer with statistics */
    len = snprintf(device_buffer, BUFFER_SIZE, 
                  "IRQ count: %d\nBottom-half count: %d\nDelayed work count: %d\n",
                  atomic_read(&irq_count),
                  atomic_read(&bottom_half_count),
                  atomic_read(&delayed_work_count));
    
    if (*offset >= len)
        return 0; /* EOF */
    
    /* Calculate bytes to read */
    bytes_to_read = min((size_t)(len - *offset), count);
    
    /* Copy data to user space */
    bytes_not_copied = copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read);
    
    /* Update file position */
    *offset += (bytes_to_read - bytes_not_copied);
    
    /* Return number of bytes successfully read */
    return (bytes_to_read - bytes_not_copied);
}

static ssize_t irq_demo_write(struct file *file, const char __user *user_buffer, 
                              size_t count, loff_t *offset)
{
    char buffer[16];
    size_t bytes_to_copy = min(count, sizeof(buffer) - 1);
    
    /* Copy from user */
    if (copy_from_user(buffer, user_buffer, bytes_to_copy))
        return -EFAULT;
    
    /* Null-terminate */
    buffer[bytes_to_copy] = '\0';
    
    /* Process the command */
    if (strncmp(buffer, "trigger", 7) == 0) {
        /* Manually trigger the interrupt handler */
        button_irq_handler(0, NULL);
        pr_info("irq_demo: Manually triggered interrupt\n");
    }
    else if (strncmp(buffer, "reset", 5) == 0) {
        /* Reset all counters */
        atomic_set(&irq_count, 0);
        atomic_set(&bottom_half_count, 0);
        atomic_set(&delayed_work_count, 0);
        pr_info("irq_demo: All counters reset\n");
    }
    
    return bytes_to_copy;
}

static int __init irq_demo_init(void)
{
    int ret = 0;
    struct proc_dir_entry *proc_file;
    
    /* Initialize synchronization primitives */
    spin_lock_init(&stats_lock);
    mutex_init(&proc_mutex);
    
    /* Initialize work items */
    INIT_WORK(&regular_work, demo_work_handler);
    INIT_DELAYED_WORK(&delayed_work, demo_delayed_work_handler);
    
    /* Create workqueue */
    demo_wq = create_workqueue("irq_demo_wq");
    if (!demo_wq) {
        pr_err("irq_demo: Failed to create workqueue\n");
        return -ENOMEM;
    }
    
    /* Schedule the delayed work */
    queue_delayed_work(demo_wq, &delayed_work, msecs_to_jiffies(5000)); /* 5 seconds */
    
    /* Initialize the timer for simulating interrupts */
    if (using_timer) {
        hrtimer_init(&demo_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        demo_timer.function = demo_timer_callback;
        hrtimer_start(&demo_timer, ns_to_ktime(TIMER_PERIOD_NS), HRTIMER_MODE_REL);
    }
    
    /* Request GPIO and set up hardware interrupt */
    if (using_gpio) {
        /* Request the GPIO */
        ret = gpio_request(BUTTON_GPIO, "button-irq");
        if (ret) {
            pr_err("irq_demo: Failed to request GPIO %d\n", BUTTON_GPIO);
            goto fail_gpio;
        }
        
        /* Set GPIO direction */
        ret = gpio_direction_input(BUTTON_GPIO);
        if (ret) {
            pr_err("irq_demo: Failed to set GPIO %d as input\n", BUTTON_GPIO);
            goto fail_gpio_direction;
        }
        
        /* Get the IRQ number for the GPIO */
        button_irq = gpio_to_irq(BUTTON_GPIO);
        if (button_irq < 0) {
            pr_err("irq_demo: Failed to get IRQ for GPIO %d\n", BUTTON_GPIO);
            ret = button_irq;
            goto fail_gpio_to_irq;
        }
        
        /* Request the IRQ */
        ret = request_irq(button_irq, button_irq_handler, 
                          IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                          "button-irq", NULL);
        if (ret) {
            pr_err("irq_demo: Failed to request IRQ %d\n", button_irq);
            goto fail_request_irq;
        }
        
        pr_info("irq_demo: GPIO %d mapped to IRQ %d\n", BUTTON_GPIO, button_irq);
    }
    
    /* Dynamically allocate a major number */
    major_number = register_chrdev(0, DEVICE_NAME, &irq_fops);
    if (major_number < 0) {
        pr_err("irq_demo: Failed to register a major number\n");
        ret = major_number;
        goto fail_register_chrdev;
    }
    pr_info("irq_demo: Registered with major number %d\n", major_number);
    
    /* Register the device class */
    irq_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(irq_class)) {
        pr_err("irq_demo: Failed to register device class\n");
        ret = PTR_ERR(irq_class);
        goto fail_class_create;
    }
    pr_info("irq_demo: Device class registered\n");
    
    /* Create the device */
    irq_device = device_create(irq_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(irq_device)) {
        pr_err("irq_demo: Failed to create the device\n");
        ret = PTR_ERR(irq_device);
        goto fail_device_create;
    }
    pr_info("irq_demo: Device created (/dev/%s)\n", DEVICE_NAME);
    
    /* Initialize character device */
    cdev_init(&irq_cdev, &irq_fops);
    irq_cdev.owner = THIS_MODULE;
    
    /* Add the character device to the system */
    ret = cdev_add(&irq_cdev, MKDEV(major_number, 0), 1);
    if (ret < 0) {
        pr_err("irq_demo: Failed to add character device\n");
        goto fail_cdev_add;
    }
    
    /* Create proc file */
    proc_file = proc_create("irq_demo", 0444, NULL, &irq_proc_fops);
    if (!proc_file) {
        pr_err("irq_demo: Failed to create proc entry\n");
        ret = -ENOMEM;
        goto fail_proc_create;
    }
    
    pr_info("irq_demo: Module loaded\n");
    return 0;

/* Error handling and cleanup */
fail_proc_create:
    cdev_del(&irq_cdev);
fail_cdev_add:
    device_destroy(irq_class, MKDEV(major_number, 0));
fail_device_create:
    class_destroy(irq_class);
fail_class_create:
    unregister_chrdev(major_number, DEVICE_NAME);
fail_register_chrdev:
    if (using_gpio)
        free_irq(button_irq, NULL);
fail_request_irq:
fail_gpio_to_irq:
fail_gpio_direction:
    if (using_gpio)
        gpio_free(BUTTON_GPIO);
fail_gpio:
    if (using_timer)
        hrtimer_cancel(&demo_timer);
    cancel_delayed_work_sync(&delayed_work);
    flush_workqueue(demo_wq);
    destroy_workqueue(demo_wq);
    
    return ret;
}

static void __exit irq_demo_exit(void)
{
    /* Cancel the timer */
    if (using_timer)
        hrtimer_cancel(&demo_timer);
    
    /* Cancel and flush work */
    cancel_delayed_work_sync(&delayed_work);
    flush_workqueue(demo_wq);
    destroy_workqueue(demo_wq);
    
    /* Free IRQ and GPIO */
    if (using_gpio) {
        free_irq(button_irq, NULL);
        gpio_free(BUTTON_GPIO);
    }
    
    /* Remove proc file */
    remove_proc_entry("irq_demo", NULL);
    
    /* Remove the character device */
    cdev_del(&irq_cdev);
    
    /* Remove the device from the system */
    device_destroy(irq_class, MKDEV(major_number, 0));
    
    /* Unregister the device class */
    class_destroy(irq_class);
    
    /* Unregister the major number */
    unregister_chrdev(major_number, DEVICE_NAME);
    
    pr_info("irq_demo: Module unloaded\n");
}

/* Register module init/exit functions */
module_init(irq_demo_init);
module_exit(irq_demo_exit); 