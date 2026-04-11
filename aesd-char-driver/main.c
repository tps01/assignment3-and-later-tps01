/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("tps01");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open\n");
    // private data is a void * pointing to aesd_dev
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release\n");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &dev->buffer;
    struct aesd_buffer_entry *temp_entry;
    size_t offset_return;

    mutex_lock(&dev->lock);
    PDEBUG("Locked mutex for read\n");

    //get entry
    temp_entry = aesd_circular_buffer_find_entry_offset_for_fpos(buffer, *f_pos, &offset_return);
    PDEBUG("Found entry with offset %ld\n", offset_return);

    //check for completely empty buffer
    if (temp_entry == NULL){
        PDEBUG("Unlocked mutex for read due to empty circular buffer\n");
        mutex_unlock(&dev->lock);
        return 0;
    }


    size_t read_size = temp_entry->size - offset_return;
    if (temp_entry->size > count) {
        read_size = count;
    }
    //entry has char *buffptr and size_t size
    //put it in the user buffer
    PDEBUG("Size of entry %ld\n", temp_entry->size);

    PDEBUG("Copying %s to user space\n", temp_entry->buffptr);
    copy_to_user(buf, temp_entry->buffptr + offset_return, read_size);


    retval = read_size;
    *f_pos = *f_pos + read_size; // move pointer to next entry
    mutex_unlock(&dev->lock);
    PDEBUG("Unlocked mutex for read\n");

    PDEBUG("read %zu bytes with offset %lld\n",count,*f_pos);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    //f_pos does nothing in this implementation.
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld\n",count,*f_pos);
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &dev->buffer;

    mutex_lock(&dev->lock);
    //filp->private_data is the buffer set up in open().
    PDEBUG("Locked mutex for write\n");

    if (buf == NULL) {
        mutex_unlock(&dev->lock);
        return retval;
    }

    if (dev->we.buffptr == NULL){ //no data
        PDEBUG("No data in buffer\n");
        dev->we.buffptr = (char *)kmalloc(count,GFP_KERNEL);
        dev->we.size = 0;
    } else { //already data from previous writes
        PDEBUG("Data already in buffer\n");
        dev->we.buffptr = (char *)krealloc(dev->we.buffptr,count+dev->we.size,GFP_KERNEL);
    }

    PDEBUG("Getting user buffer contents\n");
    copy_from_user(dev->we.buffptr + dev->we.size, buf, count);
    dev->we.size = count+dev->we.size;
    retval = count; // originally had = dev->we.size, but due to the above line that would be 
    //greater than the count passed by echo and it would error out. (This function still worked though)
    PDEBUG("our buf: %s\n", dev->we.buffptr);
    //PDEBUG("buf from user space: %s\n", buf);

    //check for newline at the end of the working entry
    if (dev->we.buffptr[dev->we.size - 1] == '\n') { 
        PDEBUG("Newline detected, adding to circular buffer\n");
        aesd_circular_buffer_add_entry(buffer, &dev->we);
        PDEBUG("Added to buffer\n");
        *f_pos = *f_pos + dev->we.size;
        dev->we.buffptr = NULL; // reset the working entry
        dev->we.size = 0;
        //kfree(dev->we.buffptr);
    } 

    //filp->private_data 
    mutex_unlock(&dev->lock);
    PDEBUG("Unlocked mutex for write\n");
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev\n", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    mutex_init(&aesd_device.lock);
    PDEBUG("Initialized buffer lock\n");
    aesd_circular_buffer_init(&aesd_device.buffer);
    PDEBUG("Initialized circular buffer\n");
    result = aesd_setup_cdev(&aesd_device);
    PDEBUG("Created char device\n");

    if( result ) {
        unregister_chrdev_region(dev, 1);
        return result;
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    PDEBUG("Deleting char device\n");
    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    if (aesd_device.we.buffptr != NULL) {
        kfree(aesd_device.we.buffptr);
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
