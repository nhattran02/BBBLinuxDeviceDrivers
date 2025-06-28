#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define DEV_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt, __func__

/* Pseudo device's memory */
char device_buffer[DEV_MEM_SIZE];

dev_t device_number;

struct cdev pcd_cdev;

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
    pr_info("pcd_lseek called with off=%lld, whence=%d\n", offset, whence);
    switch (whence) {
        case SEEK_SET:
            if (offset < 0 || offset > DEV_MEM_SIZE) {
                return -EINVAL;
            }
            filp->f_pos = offset;
            break;
        case SEEK_CUR:
            if (filp->f_pos + offset < 0 || filp->f_pos + offset > DEV_MEM_SIZE) {
                return -EINVAL; 
            }
            filp->f_pos += offset;
            break;
        case SEEK_END: 
            if (offset > 0 || -offset > DEV_MEM_SIZE) {
                return -EINVAL;
            }
            filp->f_pos = DEV_MEM_SIZE + offset;
            break;
        default:
            return -EINVAL;
    }
    pr_info("New file position: %lld\n", filp->f_pos);
    return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("pcd_read called with count=%zu, f_pos=%lld\n", count, *f_pos);

    if ((*f_pos + count) > DEV_MEM_SIZE) {
        count = DEV_MEM_SIZE - *f_pos; 
    }

    if (copy_to_user(buff, &device_buffer[*f_pos], count) != 0) {
        return -EFAULT; 
    }

    *f_pos += count;

    pr_info("Read %zu bytes from device buffer at position %lld\n", count, *f_pos);

    return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    pr_info("pcd_write called with count=%zu, f_pos=%lld\n", count, *f_pos);

    if (*f_pos + count > DEV_MEM_SIZE) {
        count = DEV_MEM_SIZE - *f_pos; 
    }

    if (count == 0) {
        pr_info("No space left to write\n");
        return -ENOMEM; 
    }

    if (copy_from_user(&device_buffer[*f_pos], buff, count) != 0) {
        return -EFAULT; 
    }

    *f_pos += count;

    pr_info("Wrote %zu bytes to device buffer at position %lld\n", count, *f_pos);
    return count;
}

int pcd_open(struct inode *inode, struct file *filp)
{
    pr_info("pcd_open called\n");
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
    pr_info("pcd_release called\n");
    return 0;
}

struct file_operations pcd_fops = {
    .owner = THIS_MODULE,
    .open = pcd_open,
    .release = pcd_release,
    .read = pcd_read,
    .write = pcd_write,
    .llseek = pcd_lseek,
};

struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_init(void)
{
    int ret;
    /* Dynamically allocate a device number */
    ret = alloc_chrdev_region(&device_number, 0, 1, "pseudo_char_device");
    if (ret < 0) {
        pr_err("Failed to allocate character device region\n");
        return ret;
    }

    pr_info("PCD number <major>:<minor> = %d:%d\n",
            MAJOR(device_number), MINOR(device_number));

    /* Initialize the cdev structure with file operations */
    cdev_init(&pcd_cdev, &pcd_fops);

    /* Register a device (cdev structure) with VFS */
    pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, device_number, 1);
    if (ret < 0) {
        pr_err("Failed to add character device\n");
        unregister_chrdev_region(device_number, 1);
        return ret;
    }

    /* Create device class under /sys/class/xyz */
    class_pcd = class_create(THIS_MODULE, "pseudo_char_class");
    if (IS_ERR(class_pcd)) {
        pr_err("Failed to create class\n");
        cdev_del(&pcd_cdev);
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(class_pcd);
    }

    /* Create a device under /dev/xyz */
    device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
    if (IS_ERR(device_pcd)) {
        pr_err("Failed to create device\n");
        class_destroy(class_pcd);
        cdev_del(&pcd_cdev);
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(device_pcd);
    }

    pr_info("Pseudo character driver initialized\n");

    return 0;
}

static void __exit pcd_exit(void)
{
    device_destroy(class_pcd, device_number);
    class_destroy(class_pcd);
    cdev_del(&pcd_cdev);
    unregister_chrdev_region(device_number, 1);
    
    pr_info("Pseudo character driver exited\n");
}

module_init(pcd_init);
module_exit(pcd_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhat Tran");
MODULE_DESCRIPTION("A simple pseudo character driver");
