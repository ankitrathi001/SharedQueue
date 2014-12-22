/******************************************************************************
 *
 * File Name: Squeue.c
 *
 * Author: Ankit Rathi (ASU ID: 1207543476)
 *
 * Date: 21-SEP-2014
 *
 * Description: A device driver for shared queues which perform the basic
 * enqueue and dequeue operations and record the accumulated queueing time.
 * 
 *****************************************************************************/
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <linux/jiffies.h>
#include "CircularBuffer.h"
#include <linux/init.h>

#define DEVICE_DRIVER_NAME "SMQDriver"
#define DEVICE_NAME1 "bus_in_q"
#define DEVICE_NAME2 "bus_out_q1"
#define DEVICE_NAME3 "bus_out_q2"
#define DEVICE_NAME4 "bus_out_q3"

/**
 * per device structure
 */
struct My_dev
{
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	CircularBuffer cb;				/* Circular Buffer*/
	struct semaphore mutex;		    /* SEMAPHORE per device */
} *bus_in_q, *bus_out_q1, *bus_out_q2, *bus_out_q3;

static dev_t my_dev_number;      /* Allotted device number */
struct class *my_dev_class;      /* Tie with the device model */


/**
 * rdtsc() function is used to calulcate the number of clock ticks
 * and measure the time. TSC(time stamp counter) is incremented 
 * every cpu tick (1/CPU_HZ).
 * 
 * Source: http://www.mcs.anl.gov/~kazutomo/rdtsc.html
 */
static __inline__ unsigned long long rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((unsigned long long) lo) | ((unsigned long long) hi)<<32;
}

/**
 * My_driver_open() method is used by driver to initialize.
 */
int My_driver_open(struct inode *inode, struct file *file)
{
	struct My_dev *my_devp;
	my_devp = container_of(inode->i_cdev, struct My_dev, cdev);			/* Get the per-device structure that contains this cdev */
	file->private_data = my_devp;										/* Easy access to cmos_devp from rest of the entry points */
	//printk("%s has opened\n", my_devp->name);
	return 0;
}

/**
 * My_driver_release() method is used by the driver to 
 * close anything which has been opened and used during driver execution.
 */
int My_driver_release(struct inode *inode, struct file *file)
{
	struct My_dev *my_devp = file->private_data;
	printk("\nMy_driver_release squeue() -- %s is closing\n", my_devp->name);
	return 0;
}

/**
 * My_driver_read() method is used to copy data from kernel to user space.
 */
static ssize_t My_driver_read(struct file *file, char *buf, size_t count, loff_t *ptr)
{
	int ret;
	int res;
	struct My_dev *my_devp = file->private_data;
	MessageToken msgtok;
	down(&(my_devp->mutex));
	ret = dequeue_CircularBuffer(&(my_devp->cb), &msgtok);

	if(ret == -1)
	{
		//printk("Buffer is empty\n");
	}
	else
	{
		if(strcmp(my_devp->name, DEVICE_NAME1))
		{
			msgtok.timeStamp1 = rdtsc() - msgtok.timeStamp1;
		}
		else
		{
			msgtok.timeStamp2 = rdtsc() - msgtok.timeStamp2;
		}
		res = copy_to_user(buf, &msgtok, sizeof(MessageToken));
		if(res)
		{
			//printk("copy to user fail \n");
			up(&(my_devp->mutex));
			return -EFAULT;
		}
	}
	up(&(my_devp->mutex));
	//printk("My_driver_read End\n");
	return ret;
}

/**
 * My_driver_write() method is used to copy data to kernel from user space.
 */
ssize_t My_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int res;
	int ret;
	MessageToken user_msgtoken;
	struct My_dev *my_devp = file->private_data;
	down(&(my_devp->mutex));
	res = copy_from_user((void *)&user_msgtoken, (void * __user)buf, count);
	if(res)
	{
		up(&(my_devp->mutex));
		return -EFAULT;
	}
	if(strcmp(my_devp->name, DEVICE_NAME1))
	{
		user_msgtoken.timeStamp1 = rdtsc();
	}
	else
	{
		user_msgtoken.timeStamp2 = rdtsc();
	}
	ret=enqueue_CircularBuffer(&(my_devp->cb), &user_msgtoken);
	if(ret == -1)
	{
		//printk("Buffer is full\n");
	}
	else
	{
	}
	up(&(my_devp->mutex));
	return ret;
}

/**
 * File operations structure. Defined in linux/fs.h
 */
static struct file_operations My_fops =
{
		.owner = THIS_MODULE,           	/* Owner */
		.open = My_driver_open,              /* Open method */
		.release = My_driver_release,        /* Release method */
		.write = My_driver_write,            /* Write method */
		.read = My_driver_read				/* Read method */
};

/**
 * My_driver_init() method is used by driver to initialize.
 */
int __init My_driver_init(void)
{
	int ret;
	
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&my_dev_number, 0, 4, DEVICE_DRIVER_NAME) < 0) //	firstminor = 1 and count = 2
	{
		printk(KERN_DEBUG "Can't register device\n");
		return -1;
	}
	printk("Squeue  My major number = %d\n", MAJOR(my_dev_number));
	
	/* Populate sysfs entries */
	my_dev_class = class_create(THIS_MODULE, DEVICE_DRIVER_NAME);
	
	/* Allocate memory for the per-device structure my_devp1 */
	bus_in_q = kmalloc(sizeof(struct My_dev), GFP_KERNEL);
	if(!bus_in_q)
	{
		printk("Bad Kmalloc bus_in_q\n");
		return -ENOMEM;
	}
	
	/* Allocate memory for the per-device structure my_devp2*/
	bus_out_q1 = kmalloc(sizeof(struct My_dev), GFP_KERNEL);
	if (!bus_out_q1)
	{
		printk("Bad Kmalloc bus_out_q1\n");
		return -ENOMEM;
	}
	
	/* Allocate memory for the per-device structure my_devp2*/
	bus_out_q2 = kmalloc(sizeof(struct My_dev), GFP_KERNEL);
	if (!bus_out_q2)
	{
		printk("Bad Kmalloc bus_out_q2\n");
		return -ENOMEM;
	}
	
	/* Allocate memory for the per-device structure my_devp2*/
	bus_out_q3 = kmalloc(sizeof(struct My_dev), GFP_KERNEL);
	if (!bus_out_q3)
	{
		printk("Bad Kmalloc bus_out_q3\n");
		return -ENOMEM;
	}
	
	/* Request I/O region */
	sprintf(bus_in_q->name, DEVICE_NAME1);
	sprintf(bus_out_q1->name, DEVICE_NAME2);
	sprintf(bus_out_q2->name, DEVICE_NAME3);
	sprintf(bus_out_q3->name, DEVICE_NAME4);

	/* Connect the file operations with the cdev */
	cdev_init(&bus_in_q->cdev, &My_fops);
	cdev_init(&bus_out_q1->cdev, &My_fops);
	cdev_init(&bus_out_q2->cdev, &My_fops);
	cdev_init(&bus_out_q3->cdev, &My_fops);
	bus_in_q->cdev.owner = THIS_MODULE;
	bus_out_q1->cdev.owner = THIS_MODULE;
	bus_out_q2->cdev.owner = THIS_MODULE;
	bus_out_q3->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&bus_in_q->cdev, MKDEV(MAJOR(my_dev_number), 0), 1);
	if(ret)
	{
		printk("Bad cdev for bus_in_q\n");
		return ret;
	}
	ret = cdev_add(&bus_out_q1->cdev, MKDEV(MAJOR(my_dev_number), 1), 1);
	if(ret)
	{
		printk("Bad cdev for bus_out_q1\n");
		return ret;
	}
	ret = cdev_add(&bus_out_q2->cdev, MKDEV(MAJOR(my_dev_number), 2), 1);
	if(ret)
	{
		printk("Bad cdev for bus_out_q2\n");
		return ret;
	}
	ret = cdev_add(&bus_out_q3->cdev, MKDEV(MAJOR(my_dev_number), 3), 1);
	if(ret)
	{
		printk("Bad cdev for bus_out_q3\n");
		return ret;
	}
	
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 0), NULL, DEVICE_NAME1);
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 1), NULL, DEVICE_NAME2);
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 2), NULL, DEVICE_NAME3);
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 3), NULL, DEVICE_NAME4);
	
	/* Initialize the Circular Buffer */
	init_CircularBuffer(&(bus_in_q->cb));
	init_CircularBuffer(&(bus_out_q1->cb));
	init_CircularBuffer(&(bus_out_q2->cb));
	init_CircularBuffer(&(bus_out_q3->cb));
	printk("Circular Buffer initialized\n");
	
	/* Initialize the semaphore */
	sema_init(&(bus_in_q->mutex),1);
	sema_init(&(bus_out_q1->mutex),1);
	sema_init(&(bus_out_q2->mutex),1);
	sema_init(&(bus_out_q3->mutex),1);
	printk("Semaphores Initialized\n");

	printk("My Driver = %s Initialized.\n", DEVICE_DRIVER_NAME);
	printk("Squeue.c My_driver_init() End \n");
	return 0;
}

/**
 * My_driver_exit() method is invked before exiting.
 */
void __exit My_driver_exit(void)
{
	printk("My_driver_exit() Start\n");
	/* Destroy device with Minor Number 0*/
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 0));
	cdev_del(&bus_in_q->cdev);
	kfree(bus_in_q);
	
	/* Destroy device with Minor Number 1*/
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 1));
	cdev_del(&bus_out_q1->cdev);
	kfree(bus_out_q1);
	
	/* Destroy device with Minor Number 2*/
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 2));
	cdev_del(&bus_out_q2->cdev);
	kfree(bus_out_q2);
	
	/* Destroy device with Minor Number 3*/
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 3));
	cdev_del(&bus_out_q3->cdev);
	kfree(bus_out_q3);
	
	/* Destroy driver_class */
	class_destroy(my_dev_class);

	/* Release the major number */
	unregister_chrdev_region((my_dev_number), 1);
	printk("My_driver_exit() End\n");
}

module_init(My_driver_init);
module_exit(My_driver_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ankit Rathi");
