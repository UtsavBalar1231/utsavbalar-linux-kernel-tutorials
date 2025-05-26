#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/rwsem.h>
#include <linux/atomic.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h> /* For LINUX_VERSION_CODE */

#define DEVICE_NAME "sync_demo"
#define CLASS_NAME "sync"
#define BUFFER_SIZE 1024
#define NUM_COUNTERS 4

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Utsav Balar");
MODULE_DESCRIPTION("Synchronization primitives demonstration module");
MODULE_VERSION("0.1");

/* Define our synchronization primitives */
static atomic_t atomic_counter = ATOMIC_INIT(0);
static spinlock_t spin_counter_lock;
static struct mutex mutex_counter_lock;
static struct semaphore sem_counter_lock;
static struct rw_semaphore rwsem_counter_lock;

/* Counters to demonstrate the primitives */
static int counter_values[NUM_COUNTERS]; // [atomic, spinlock, mutex, semaphore]

/* Device variables */
static int major_number;
static struct class *sync_class = NULL;
static struct device *sync_device = NULL;
static struct cdev sync_cdev;
static char device_buffer[BUFFER_SIZE];

/* Thread-related variables */
static struct task_struct *demo_thread = NULL;
static bool thread_should_stop = false;

/* Forward declarations */
static int sync_demo_open(struct inode *, struct file *);
static int sync_demo_release(struct inode *, struct file *);
static ssize_t sync_demo_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t sync_demo_write(struct file *, const char __user *, size_t,
			       loff_t *);

/* File operations */
static struct file_operations sync_fops = {
	.owner = THIS_MODULE,
	.open = sync_demo_open,
	.release = sync_demo_release,
	.read = sync_demo_read,
	.write = sync_demo_write,
};

/* ProcFS handler */
static int sync_proc_show(struct seq_file *m, void *v)
{
	/* Read atomic counter */
	int atomic_value = atomic_read(&atomic_counter);

	/* Read other counters with their respective synchronization mechanisms */
	int spin_value, mutex_value, sem_value, rwsem_value;

	/* Spinlock */
	spin_lock(&spin_counter_lock);
	spin_value = counter_values[1];
	spin_unlock(&spin_counter_lock);

	/* Mutex */
	mutex_lock(&mutex_counter_lock);
	mutex_value = counter_values[2];
	mutex_unlock(&mutex_counter_lock);

	/* Semaphore */
	down(&sem_counter_lock);
	sem_value = counter_values[3];
	up(&sem_counter_lock);

	/* Read-write semaphore (read lock) */
	down_read(&rwsem_counter_lock);
	rwsem_value =
		counter_values[0]; /* Using the first counter for rwsem demo */
	up_read(&rwsem_counter_lock);

	/* Output the values */
	seq_puts(m, "Synchronization Primitives Demo\n");
	seq_puts(m, "==============================\n\n");

	seq_printf(m, "1. Atomic counter: %d\n", atomic_value);
	seq_printf(m, "2. Spinlock counter: %d\n", spin_value);
	seq_printf(m, "3. Mutex counter: %d\n", mutex_value);
	seq_printf(m, "4. Semaphore counter: %d\n", sem_value);
	seq_printf(m, "5. RW Semaphore counter: %d (shared with atomic)\n",
		   rwsem_value);

	return 0;
}

static int sync_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sync_proc_show, NULL);
}

static const struct proc_ops sync_proc_fops = {
	.proc_open = sync_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* Thread function to demonstrate concurrency */
static int demo_thread_fn(void *data)
{
	/* Name our thread */
	set_current_state(TASK_RUNNING);

	pr_info("sync_demo: Background thread started\n");

	/* Run until module is unloaded */
	while (!kthread_should_stop() && !thread_should_stop) {
		/* Increment atomic counter */
		atomic_inc(&atomic_counter);

		/* Increment spinlock counter */
		spin_lock(&spin_counter_lock);
		counter_values[1]++;
		spin_unlock(&spin_counter_lock);

		/* Increment mutex counter */
		mutex_lock(&mutex_counter_lock);
		counter_values[2]++;
		mutex_unlock(&mutex_counter_lock);

		/* Increment semaphore counter */
		down(&sem_counter_lock);
		counter_values[3]++;
		up(&sem_counter_lock);

		/* Update the rwsem counter with a write lock */
		down_write(&rwsem_counter_lock);
		counter_values[0] =
			atomic_read(&atomic_counter); /* Sync with atomic */
		up_write(&rwsem_counter_lock);

		/* Sleep to demonstrate that other code can run */
		msleep(1000); /* Sleep for 1 second */
	}

	pr_info("sync_demo: Background thread stopped\n");
	return 0;
}

/* Character device functions */
static int sync_demo_open(struct inode *inode, struct file *file)
{
	/* Nothing special to do here */
	return 0;
}

static int sync_demo_release(struct inode *inode, struct file *file)
{
	/* Nothing special to do here */
	return 0;
}

static ssize_t sync_demo_read(struct file *file, char __user *user_buffer,
			      size_t count, loff_t *offset)
{
	int bytes_to_read;
	int bytes_not_copied;
	int len;

	/* Prepare the buffer */
	len = snprintf(
		device_buffer, BUFFER_SIZE,
		"Atomic counter: %d\nSpinlock counter: %d\nMutex counter: %d\nSemaphore counter: %d\n",
		atomic_read(&atomic_counter), counter_values[1],
		counter_values[2], counter_values[3]);

	if (*offset >= len)
		return 0; /* EOF */

	/* Calculate bytes to read */
	bytes_to_read = min((size_t)(len - *offset), count);

	/* Copy data to user space */
	bytes_not_copied = copy_to_user(user_buffer, device_buffer + *offset,
					bytes_to_read);

	/* Update file position */
	*offset += (bytes_to_read - bytes_not_copied);

	/* Return number of bytes successfully read */
	return (bytes_to_read - bytes_not_copied);
}

static ssize_t sync_demo_write(struct file *file,
			       const char __user *user_buffer, size_t count,
			       loff_t *offset)
{
	char buffer[16];
	size_t bytes_to_copy = min(count, sizeof(buffer) - 1);

	/* Copy from user */
	if (copy_from_user(buffer, user_buffer, bytes_to_copy))
		return -EFAULT;

	/* Null-terminate */
	buffer[bytes_to_copy] = '\0';

	/* Process the command */
	if (strncmp(buffer, "reset", 5) == 0) {
		/* Reset all counters */
		atomic_set(&atomic_counter, 0);

		spin_lock(&spin_counter_lock);
		counter_values[1] = 0;
		spin_unlock(&spin_counter_lock);

		mutex_lock(&mutex_counter_lock);
		counter_values[2] = 0;
		mutex_unlock(&mutex_counter_lock);

		down(&sem_counter_lock);
		counter_values[3] = 0;
		up(&sem_counter_lock);

		down_write(&rwsem_counter_lock);
		counter_values[0] = 0;
		up_write(&rwsem_counter_lock);

		pr_info("sync_demo: All counters reset\n");
	}

	return bytes_to_copy;
}

static int __init sync_demo_init(void)
{
	int ret = 0;
	struct proc_dir_entry *proc_file;

	/* Initialize counters */
	memset(counter_values, 0, sizeof(counter_values));

	/* Initialize synchronization primitives */
	spin_lock_init(&spin_counter_lock);
	mutex_init(&mutex_counter_lock);
	sema_init(&sem_counter_lock, 1); /* Binary semaphore */
	init_rwsem(&rwsem_counter_lock);

	/* Dynamically allocate a major number */
	major_number = register_chrdev(0, DEVICE_NAME, &sync_fops);
	if (major_number < 0) {
		pr_err("sync_demo: Failed to register a major number\n");
		return major_number;
	}
	pr_info("sync_demo: Registered with major number %d\n", major_number);

	/* Register the device class - with version-specific API handling */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
	/* New API for Linux 6.12 and later */
	sync_class = class_create(CLASS_NAME);
#else
	/* Legacy API for Linux kernels before 6.12 */
	sync_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
	if (IS_ERR(sync_class)) {
		unregister_chrdev(major_number, DEVICE_NAME);
		pr_err("sync_demo: Failed to register device class\n");
		return PTR_ERR(sync_class);
	}
	pr_info("sync_demo: Device class registered\n");

	/* Create the device */
	sync_device = device_create(sync_class, NULL, MKDEV(major_number, 0),
				    NULL, DEVICE_NAME);
	if (IS_ERR(sync_device)) {
		class_destroy(sync_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		pr_err("sync_demo: Failed to create the device\n");
		return PTR_ERR(sync_device);
	}
	pr_info("sync_demo: Device created (/dev/%s)\n", DEVICE_NAME);

	/* Initialize character device */
	cdev_init(&sync_cdev, &sync_fops);
	sync_cdev.owner = THIS_MODULE;

	/* Add the character device to the system */
	ret = cdev_add(&sync_cdev, MKDEV(major_number, 0), 1);
	if (ret < 0) {
		device_destroy(sync_class, MKDEV(major_number, 0));
		class_destroy(sync_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		pr_err("sync_demo: Failed to add character device\n");
		return -EFAULT;
	}

	/* Create proc file */
	proc_file = proc_create("sync_demo", 0444, NULL, &sync_proc_fops);
	if (!proc_file) {
		cdev_del(&sync_cdev);
		device_destroy(sync_class, MKDEV(major_number, 0));
		class_destroy(sync_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		pr_err("sync_demo: Failed to create proc entry\n");
		return -ENOMEM;
	}

	/* Start the demo thread */
	demo_thread = kthread_run(demo_thread_fn, NULL, "sync_demo_thread");
	if (IS_ERR(demo_thread)) {
		remove_proc_entry("sync_demo", NULL);
		cdev_del(&sync_cdev);
		device_destroy(sync_class, MKDEV(major_number, 0));
		class_destroy(sync_class);
		unregister_chrdev(major_number, DEVICE_NAME);
		pr_err("sync_demo: Failed to create kernel thread\n");
		return PTR_ERR(demo_thread);
	}

	pr_info("sync_demo: Module loaded\n");
	return ret;
}

static void __exit sync_demo_exit(void)
{
	/* Stop the demo thread */
	thread_should_stop = true;
	if (demo_thread)
		kthread_stop(demo_thread);

	/* Remove the proc file */
	remove_proc_entry("sync_demo", NULL);

	/* Remove the character device */
	cdev_del(&sync_cdev);

	/* Remove the device from the system */
	device_destroy(sync_class, MKDEV(major_number, 0));

	/* Unregister the device class */
	class_destroy(sync_class);

	/* Unregister the major number */
	unregister_chrdev(major_number, DEVICE_NAME);

	pr_info("sync_demo: Module unloaded\n");
}

/* Register module init/exit functions */
module_init(sync_demo_init);
module_exit(sync_demo_exit);
