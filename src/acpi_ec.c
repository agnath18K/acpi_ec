/*
 * ACPI EC Driver - Embedded Controller interface
 * 
 * This file is an altered version of ec_sys.c from the Linux kernel,
 * modified to work as an out-of-tree module without using debugfs.
 * 
 * Enhanced with multi-EC support, including:
 * - Device enumeration for multiple controllers
 * - Independent synchronization for each EC
 * - Proper read/write operations across controllers
 * 
 * Authors:
 * - Thomas Renninger <trenn@suse.de> (Original author)
 * - Sayafdine Said <musikid@outlook.com> (Out-of-tree port)
 * - Arun Gopinath <agnath18@gmail.com> (Multi-EC support)
 *
 * Original copyright:
 * Copyright (C) 2010 SUSE Products GmbH/Novell
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "acpi_ec.h"

#define DRV_NAME "acpi_ec"
#define DRV_VERSION "1.0.5"
#define DRV_DESC "ACPI EC driver with multi-EC support"

MODULE_AUTHOR("Thomas Renninger <trenn@suse.de>");
MODULE_AUTHOR("Sayafdine Said <musikid@outlook.com>");
MODULE_AUTHOR("Arun Gopinath <agnath18@gmail.com>");
MODULE_DESCRIPTION("ACPI EC access driver");
MODULE_DESCRIPTION("Enhanced with multi-EC support for independent controller access");
MODULE_LICENSE("GPL");

#define EC_SPACE_SIZE 256
#define MAX_EC_DEVICES 8

static LIST_HEAD(ec_devices);
static struct class *dev_class;

static ssize_t acpi_ec_read(struct file *f, char __user *buf, size_t count,
                            loff_t *off) {
    struct ec_device *ec_dev = f->private_data;
    unsigned int size = EC_SPACE_SIZE;
    loff_t init_off = *off;
    int err = 0;

    if (!ec_dev || !ec_dev->ec)
        return -ENODEV;

    if (*off >= size)
        return 0;

    if (*off + count >= size) {
        size -= *off;
        count = size;
    } else
        size = count;

    while (size) {
        u8 byte_read;
        err = ec_read_byte(ec_dev->ec, *off, &byte_read);
        if (err)
            return err;

        if (put_user(byte_read, buf + *off - init_off)) {
            if (*off - init_off)
                return *off - init_off; /* partial read */
            return -EFAULT;
        }

        *off += 1;
        size--;
    }
    return count;
}

static ssize_t acpi_ec_write(struct file *f, const char __user *buf,
                             size_t count, loff_t *off) {
    struct ec_device *ec_dev = f->private_data;
    unsigned int size = count;
    loff_t init_off = *off;
    int err = 0;

    if (!ec_dev || !ec_dev->ec)
        return -ENODEV;

    if (*off >= EC_SPACE_SIZE)
        return 0;

    if (*off + count >= EC_SPACE_SIZE) {
        size = EC_SPACE_SIZE - *off;
        count = size;
    }

    while (size) {
        u8 byte_write;
        if (get_user(byte_write, buf + *off - init_off)) {
            if (*off - init_off)
                return *off - init_off; /* partial write */
            return -EFAULT;
        }
        err = ec_write_byte(ec_dev->ec, *off, byte_write);
        if (err)
            return err;

        *off += 1;
        size--;
    }
    return count;
}

static int acpi_ec_open(struct inode *inode, struct file *file)
{
    struct ec_device *ec_dev = container_of(inode->i_cdev, struct ec_device, cdev);
    file->private_data = ec_dev;
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = acpi_ec_open,
    .read = acpi_ec_read,
    .write = acpi_ec_write,
    .llseek = default_llseek,
};

static int acpi_ec_create_dev(struct acpi_ec *ec, int index)
{
    struct ec_device *ec_dev;
    int err = -1;
    char dev_name[16];

    ec_dev = kzalloc(sizeof(*ec_dev), GFP_KERNEL);
    if (!ec_dev) {
        printk(KERN_ERR "acpi_ec: Failed to allocate ec_device\n");
        return -ENOMEM;
    }

    ec_dev->ec = ec;
    ec_dev->index = index;
    INIT_LIST_HEAD(&ec_dev->list);

    if ((err = alloc_chrdev_region(&ec_dev->dev, 0, 1, DEVICE_NAME)) < 0) {
        printk(KERN_ERR "acpi_ec: Failed to allocate a char_dev region\n");
        goto error;
    }

    snprintf(dev_name, sizeof(dev_name), "%s%d", DEVICE_NAME, index);
    if (IS_ERR(device_create(dev_class, NULL, ec_dev->dev, NULL, dev_name))) {
        printk(KERN_ERR "acpi_ec: Failed to create device %s\n", dev_name);
        err = -1;
        goto error;
    }

    cdev_init(&ec_dev->cdev, &fops);
    if ((err = cdev_add(&ec_dev->cdev, ec_dev->dev, 1)) < 0) {
        printk(KERN_ERR "acpi_ec: Failed to add device %s\n", dev_name);
        device_destroy(dev_class, ec_dev->dev);
        goto error;
    }

    list_add_tail(&ec_dev->list, &ec_devices);
    return 0;

error:
    if (err >= 0)
        unregister_chrdev_region(ec_dev->dev, 1);
    kfree(ec_dev);
    return err;
}

static void acpi_ec_remove_dev(struct ec_device *ec_dev)
{
    list_del(&ec_dev->list);
    cdev_del(&ec_dev->cdev);
    device_destroy(dev_class, ec_dev->dev);
    unregister_chrdev_region(ec_dev->dev, 1);
    kfree(ec_dev);
}

static int __init acpi_ec_init(void)
{
    struct acpi_ec *ec;
    int err = -1;
    int index = 0;

    if (IS_ERR(dev_class = class_create(DEVICE_NAME))) {
        printk(KERN_ERR "acpi_ec: Failed to create class\n");
        return -1;
    }

    /* Check if we have any EC devices */
    if (!first_ec) {
        printk(KERN_ERR "acpi_ec: No EC devices found\n");
        err = -ENODEV;
        goto error;
    }

    /* Create device for the first EC */
    err = acpi_ec_create_dev(first_ec, index++);
    if (err < 0) {
        printk(KERN_ERR "acpi_ec: Failed to create device for EC %d\n", index - 1);
        goto error;
    }

    /* Try to find additional ECs */
    for (ec = first_ec->next; ec && index < MAX_EC_DEVICES; ec = ec->next) {
        err = acpi_ec_create_dev(ec, index++);
        if (err < 0) {
            printk(KERN_ERR "acpi_ec: Failed to create device for EC %d\n", index - 1);
            goto error;
        }
    }

    printk(KERN_INFO "acpi_ec: Successfully created %d EC device(s)\n", index);
    return 0;

error:
    while (!list_empty(&ec_devices)) {
        struct ec_device *ec_dev = list_first_entry(&ec_devices, struct ec_device, list);
        acpi_ec_remove_dev(ec_dev);
    }
    class_destroy(dev_class);
    return err;
}

static void __exit acpi_ec_exit(void)
{
    while (!list_empty(&ec_devices)) {
        struct ec_device *ec_dev = list_first_entry(&ec_devices, struct ec_device, list);
        acpi_ec_remove_dev(ec_dev);
    }
    class_destroy(dev_class);
}

module_init(acpi_ec_init);
module_exit(acpi_ec_exit);
