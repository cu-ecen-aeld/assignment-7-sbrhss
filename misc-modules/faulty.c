/*
 * faulty.c -- a module which generates an oops when read
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: faulty.c,v 1.3 2004/09/26 07:02:43 gregkh Exp $
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/types.h>  /* size_t */
#include <linux/uaccess.h>
#include <linux/device.h> /* for class_create, device_create */
#include <linux/cdev.h>   /* for cdev */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("SBRHSS");
MODULE_DESCRIPTION("AESD Faulty Module - Assignment 7");
MODULE_VERSION("1.0");

static int faulty_major = 0;
static struct class *faulty_class = NULL;
static struct device *faulty_device = NULL;
static dev_t dev_num;

ssize_t faulty_read(struct file *filp, char __user *buf,
		    size_t count, loff_t *pos)
{
	int i;
	int ret;
	char stack_buf[4];
	
	printk(KERN_ALERT "faulty_read module called .... \n");
	/* Let's try a buffer overflow  */
	for (i = 0; i < 20; i++)
		*(stack_buf + i) = 0xff;
	if (count > 4)
		count = 4; /* copy 4 bytes to the user */
	ret = copy_to_user(buf, stack_buf, count);
	if (!ret)
		return count;
	return ret;
}

ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
		loff_t *pos)
{
	printk(KERN_ALERT "faulty_write module called .... \n");
	/* make a simple fault by dereferencing a NULL pointer */
	*(int *)0 = 0;
	return 0;
}

struct file_operations faulty_fops = {
	.read =  faulty_read,
	.write = faulty_write,
	.owner = THIS_MODULE
};

static int __init faulty_init(void)
{
	int result;
	dev_t dev = 0;

	printk(KERN_ALERT "faulty_init module initialized .... \n");
	
	/* Allocate a major number dynamically */
	result = alloc_chrdev_region(&dev, 0, 1, "aesd-faulty");
	if (result < 0) {
		printk(KERN_ERR "faulty: Failed to allocate chrdev region\n");
		return result;
	}
	
	faulty_major = MAJOR(dev);
	dev_num = dev;
	
	/* Create device class */
	faulty_class = class_create(THIS_MODULE, "aesd-faulty");
	if (IS_ERR(faulty_class)) {
		printk(KERN_ERR "faulty: Failed to create device class\n");
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(faulty_class);
	}
	
	/* Create device file - this creates /dev/faulty */
	faulty_device = device_create(faulty_class, NULL, dev_num, NULL, "faulty");
	if (IS_ERR(faulty_device)) {
		printk(KERN_ERR "faulty: Failed to create device\n");
		class_destroy(faulty_class);
		unregister_chrdev_region(dev, 1);
		return PTR_ERR(faulty_device);
	}
	
	/* Register character device */
	result = register_chrdev_region(dev, 1, "aesd-faulty");
	if (result < 0) {
		printk(KERN_ERR "faulty: Failed to register chrdev\n");
		device_destroy(faulty_class, dev_num);
		class_destroy(faulty_class);
		unregister_chrdev_region(dev, 1);
		return result;
	}
	
	printk(KERN_INFO "faulty: Module loaded, device created at /dev/faulty (major %d)\n", faulty_major);
	return 0;
}

static void __exit faulty_cleanup(void)
{
	printk(KERN_ALERT "faulty_cleanup module called .... \n");
	
	/* Destroy device file */
	if (faulty_device) {
		device_destroy(faulty_class, dev_num);
	}
	
	/* Destroy device class */
	if (faulty_class) {
		class_destroy(faulty_class);
	}
	
	/* Unregister character device */
	if (faulty_major > 0) {
		unregister_chrdev_region(dev_num, 1);
	}
}

module_init(faulty_init);
module_exit(faulty_cleanup);
