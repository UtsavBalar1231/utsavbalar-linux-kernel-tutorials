#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>         /* For register_chrdev, file_operations */
#include <linux/uaccess.h>    /* For copy_to_user, copy_from_user */
#include <linux/device.h>     /* For device_create, class_create */
#include <linux/cdev.h>       /* For cdev_init, cdev_add */

#define DEVICE_NAME "simple_char"
#define CLASS_NAME "simple"
#define BUFFER_SIZE 1024

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Utsav Balar");
MODULE_DESCRIPTION("A simple character device driver example");
MODULE_VERSION("0.1");

/* Global variables for our device */
static int major_number;              /* Will store our device's major number */
static char device_buffer[BUFFER_SIZE]; /* Memory buffer for the device */
static struct class *simple_class = NULL;  /* Device class */
static struct device *simple_device = NULL; /* Device */
static struct cdev simple_cdev;        /* Character device structure */

/* Prototypes for device functions */
static int simple_open(struct inode *, struct file *);
static int simple_release(struct inode *, struct file *);
static ssize_t simple_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t simple_write(struct file *, const char __user *, size_t, loff_t *);
static loff_t simple_llseek(struct file *, loff_t, int);

/* Define file operations for our device */
static struct file_operations simple_fops = {
    .owner = THIS_MODULE,
    .open = simple_open,
    .release = simple_release,
    .read = simple_read,
    .write = simple_write,
    .llseek = simple_llseek,
};

/* Called when device is opened */
static int simple_open(struct inode *inode, struct file *file)
{
    /* Nothing special to do here */
    printk(KERN_INFO "SIMPLE: Device opened\n");
    return 0;
}

/* Called when device is closed */
static int simple_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "SIMPLE: Device closed\n");
    return 0;
}

/* Called when user reads from the device */
static ssize_t simple_read(struct file *file, char __user *user_buffer, 
                          size_t count, loff_t *offset)
{
    int bytes_to_read;
    int bytes_not_copied;
    
    /* Calculate bytes to read */
    bytes_to_read = min((size_t)(BUFFER_SIZE - *offset), count);
    
    if (bytes_to_read <= 0)
        return 0; /* EOF */
    
    /* Copy data to user space */
    bytes_not_copied = copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read);
    
    /* Update file position */
    *offset += (bytes_to_read - bytes_not_copied);
    
    printk(KERN_INFO "SIMPLE: Read %d bytes\n", bytes_to_read - bytes_not_copied);
    
    /* Return number of bytes successfully read */
    return (bytes_to_read - bytes_not_copied);
}

/* Called when user writes to the device */
static ssize_t simple_write(struct file *file, const char __user *user_buffer, 
                           size_t count, loff_t *offset)
{
    int bytes_to_write;
    int bytes_not_copied;
    
    /* Calculate bytes to write */
    bytes_to_write = min((size_t)(BUFFER_SIZE - *offset), count);
    
    if (bytes_to_write <= 0)
        return -ENOSPC; /* No space left on device */
    
    /* Copy data from user space */
    bytes_not_copied = copy_from_user(device_buffer + *offset, user_buffer, bytes_to_write);
    
    /* Update file position */
    *offset += (bytes_to_write - bytes_not_copied);
    
    printk(KERN_INFO "SIMPLE: Wrote %d bytes\n", bytes_to_write - bytes_not_copied);
    
    /* Return number of bytes successfully written */
    return (bytes_to_write - bytes_not_copied);
}

/* Called when user changes file position with lseek */
static loff_t simple_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;
    
    switch(whence) {
        case SEEK_SET:  /* Set position from start of file */
            new_pos = offset;
            break;
        case SEEK_CUR:  /* Set position from current position */
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:  /* Set position from end of file */
            new_pos = BUFFER_SIZE + offset;
            break;
        default:
            return -EINVAL; /* Invalid argument */
    }
    
    if (new_pos < 0 || new_pos > BUFFER_SIZE)
        return -EINVAL; /* Invalid position */
    
    file->f_pos = new_pos;
    return new_pos;
}

/* Module initialization function */
static int __init simple_char_init(void)
{
    /* Dynamically allocate a major number */
    major_number = register_chrdev(0, DEVICE_NAME, &simple_fops);
    if (major_number < 0) {
        printk(KERN_ALERT "SIMPLE: Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "SIMPLE: Registered with major number %d\n", major_number);
    
    /* Register the device class */
    simple_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(simple_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "SIMPLE: Failed to register device class\n");
        return PTR_ERR(simple_class);
    }
    printk(KERN_INFO "SIMPLE: Device class registered\n");
    
    /* Create the device */
    simple_device = device_create(simple_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(simple_device)) {
        class_destroy(simple_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "SIMPLE: Failed to create the device\n");
        return PTR_ERR(simple_device);
    }
    printk(KERN_INFO "SIMPLE: Device created (/dev/%s)\n", DEVICE_NAME);
    
    /* Initialize character device */
    cdev_init(&simple_cdev, &simple_fops);
    simple_cdev.owner = THIS_MODULE;
    
    /* Add the character device to the system */
    if (cdev_add(&simple_cdev, MKDEV(major_number, 0), 1) < 0) {
        device_destroy(simple_class, MKDEV(major_number, 0));
        class_destroy(simple_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "SIMPLE: Failed to add character device\n");
        return -EFAULT;
    }
    
    /* Initialize the device buffer */
    memset(device_buffer, 0, BUFFER_SIZE);
    
    printk(KERN_INFO "SIMPLE: Character device driver initialized\n");
    return 0;
}

/* Module cleanup function */
static void __exit simple_char_exit(void)
{
    /* Remove the character device */
    cdev_del(&simple_cdev);
    
    /* Remove the device from the system */
    device_destroy(simple_class, MKDEV(major_number, 0));
    
    /* Unregister the device class */
    class_destroy(simple_class);
    
    /* Unregister the major number */
    unregister_chrdev(major_number, DEVICE_NAME);
    
    printk(KERN_INFO "SIMPLE: Character device driver removed\n");
}

/* Register module init/exit functions */
module_init(simple_char_init);
module_exit(simple_char_exit); 