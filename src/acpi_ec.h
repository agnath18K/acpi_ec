#ifndef _ACPI_EC_H
#define _ACPI_EC_H

/*
 * ACPI EC Driver - Embedded Controller interface
 * 
 * Add support for multiple EC devices,
 * including proper device enumeration, synchronization, and independent
 * read/write operations for each controller.
 */

#include <linux/acpi.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <acpi/actypes.h>

#define EC_SPACE_SIZE 256
#define MAX_EC_DEVICES 8
#define DEVICE_NAME "ec"

/* EC commands */
#define EC_CMD_READ 0x80
#define EC_CMD_WRITE 0x81
#define EC_CMD_BURST_ENABLE 0x82
#define EC_CMD_BURST_DISABLE 0x83
#define EC_CMD_QUERY 0x84

/* EC status register bits */
#define EC_FLAG_OBF 0x01    /* Output buffer full */
#define EC_FLAG_IBF 0x02    /* Input buffer full */
#define EC_FLAG_CMD 0x08    /* Last write was a command */
#define EC_FLAG_BURST 0x10  /* Burst mode */
#define EC_FLAG_SCI 0x20    /* SCI event */
#define EC_FLAG_SMI 0x40    /* SMI event */

/* ACPI EC structure */
struct acpi_ec {
    acpi_handle handle;
    unsigned long gpe;
    unsigned long command_addr;
    unsigned long data_addr;
    unsigned long global_lock;
    struct mutex lock;
    atomic_t query_pending;
    atomic_t event_count;
    wait_queue_head_t wait;
    struct list_head list;
    struct acpi_ec *next;
};

struct ec_device {
    struct acpi_ec *ec;
    dev_t dev;
    struct cdev cdev;
    struct device *device;
    struct list_head list;
    int index;              /* EC index for multi-EC systems */
};

/* ACPI EC functions */
extern struct acpi_ec *first_ec;

/* Helper functions for EC access */
static inline int ec_wait_write(struct acpi_ec *ec)
{
    unsigned long delay = jiffies + HZ;
    while (time_before(jiffies, delay)) {
        if (!(inb(ec->command_addr) & EC_FLAG_IBF))
            return 0;
        cpu_relax();
    }
    return -ETIME;
}

static inline int ec_wait_read(struct acpi_ec *ec)
{
    unsigned long delay = jiffies + HZ;
    while (time_before(jiffies, delay)) {
        if (inb(ec->command_addr) & EC_FLAG_OBF)
            return 0;
        cpu_relax();
    }
    return -ETIME;
}

static inline int ec_read_byte(struct acpi_ec *ec, u8 addr, u8 *val)
{
    int err;

    if (!ec || !val)
        return -EINVAL;

    err = ec_wait_write(ec);
    if (err)
        return err;

    outb(EC_CMD_READ, ec->command_addr);

    err = ec_wait_write(ec);
    if (err)
        return err;

    outb(addr, ec->data_addr);

    err = ec_wait_read(ec);
    if (err)
        return err;

    *val = inb(ec->data_addr);
    return 0;
}

static inline int ec_write_byte(struct acpi_ec *ec, u8 addr, u8 val)
{
    int err;

    if (!ec)
        return -EINVAL;

    err = ec_wait_write(ec);
    if (err)
        return err;

    outb(EC_CMD_WRITE, ec->command_addr);

    err = ec_wait_write(ec);
    if (err)
        return err;

    outb(addr, ec->data_addr);

    err = ec_wait_write(ec);
    if (err)
        return err;

    outb(val, ec->data_addr);

    return ec_wait_write(ec);
}

#endif /* _ACPI_EC_H */ 