#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#undef pr_fmt 
#define pr_fmt(fmt) "%s: " fmt, __func__

#define DEV_MEM_SIZE    (512)

/* Pseudo device's memory */ 
char device_buffer[DEV_MEM_SIZE] = {0};

/* This holds the device number */
dev_t device_number;

/* Cdev variable */
struct cdev pcd_cdev;

/* Holds the class pointer */
struct class *pcd_class;

struct device *pcd_device;

static int pcd_open(struct inode *inode, struct file *file);
static loff_t pcd_lseek(struct file *file, loff_t offset, int whence);
static int pcd_release(struct inode *inode, struct file *file);
static ssize_t pcd_read(struct file *file, char __user *buf,
                        size_t nbytes, loff_t *ppos);
static ssize_t pcd_write(struct file *file, const char __user *buf,
                        size_t nbytes, loff_t *ppos);

/* File operators for pcd devices */ 
static const struct file_operations pcd_fops = {
    .owner      = THIS_MODULE,
    .open       = pcd_open,
    .read       = pcd_read,
    .llseek     = pcd_lseek,
    .release    = pcd_release,
    .write      = pcd_write,
};


static int pcd_open(struct inode *inode, struct file *file)
{
    pr_info("PCD was opened successfully\n");
    return 0;
}

static loff_t pcd_lseek(struct file *file, loff_t offset, int whence)
{
    loff_t temp;

    pr_info("lseek requested\n");
    pr_info("Current value of the file position = %lld\n", file->f_pos);

    switch(whence)
    {
        case SEEK_SET:
            if (offset > DEV_MEM_SIZE || offset < 0) {
                return -EINVAL;
            }
            file->f_pos = offset;
            break;
        case SEEK_CUR:
            temp = file->f_pos + offset;
            if (temp > DEV_MEM_SIZE || temp < 0) {
                return -EINVAL;
            }
            file->f_pos = temp;
            break;
        case SEEK_END:
            temp = DEV_MEM_SIZE + offset; /* Offset should be negative */
            if (temp > DEV_MEM_SIZE || temp < 0) {
                return -EINVAL;
            }
            file->f_pos = temp;
            break;        
        default:
            return -EINVAL;
    }

    pr_info("New value of the file position = %lld\n", file->f_pos);

    return file->f_pos;
}

static int pcd_release(struct inode *inode, struct file *file)
{
    pr_info("PCD was released successfully\n");
    return 0;
}

static ssize_t pcd_read(struct file *file, char __user *buf,
                        size_t nbytes, loff_t *ppos)
{
    pr_info("Read requested for %zu bytes\n", nbytes);
    pr_info("Current file position = %lld\n", *ppos);

    /* Check the file posision */
    if ((*ppos + nbytes) > DEV_MEM_SIZE) {
        nbytes = DEV_MEM_SIZE - *ppos;
    }

    /* Copy to user space */
    if (copy_to_user(buf, &device_buffer[*ppos], nbytes)) {
        return -EFAULT;
    }

    /* Update the current file position */
    *ppos += nbytes;

    pr_info("Number of bytes successfully read = %zu\n",nbytes);
    pr_info("Updated file position = %lld\n",*ppos);

    return nbytes;
}

static ssize_t pcd_write(struct file *file, const char __user *buf,
                        size_t nbytes, loff_t *ppos)
{
    pr_info("Write requested for %zu bytes\n", nbytes);
    pr_info("Current file position = %lld\n", *ppos);

    if ((*ppos + nbytes) > DEV_MEM_SIZE) {
        nbytes = DEV_MEM_SIZE - *ppos;
    }

    if (!nbytes) {
        pr_err("No space left on the device\n");
        return -ENOMEM;
    }

    if (copy_from_user(&device_buffer[*ppos], buf, nbytes)) {
        return -EFAULT;
    }

    *ppos += nbytes;

    pr_info("Number of bytes successfully written = %zu\n", nbytes);
    pr_info("Updated file position = %lld\n", *ppos);

    return nbytes;
}

static int __init pcd_driver_init(void)
{
    int ret = 0;

    /* Dynamically allocate a device number */
    ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
    if (ret < 0) {
        pr_err("Allocate chrdev failed\n");
        goto out;
    }

    /* Initialize the cdev structure with file operators */
    cdev_init(&pcd_cdev, &pcd_fops);

    /* Register a device (cdev structure) with VFS */
    pcd_cdev.owner = THIS_MODULE;
    ret = cdev_add(&pcd_cdev, device_number, 1);
    if (ret < 0) {
        pr_err("Cdev add failed\n");
        goto unreg_chrdev;
    }

    /* Create device class under /dev/class/ */
    pcd_class = class_create(THIS_MODULE, "pcd_class");
    if (IS_ERR(pcd_class)) {
        pr_err("Class create failed\n");
        ret = PTR_ERR(pcd_class);
        goto cdev_del;
    }

    /* Populate the sysfs with device information */
    pcd_device = device_create(pcd_class, NULL, device_number, NULL, "pcd");
    if (IS_ERR(pcd_device)) {
        pr_err("Device create failed\n");
        ret = PTR_ERR(pcd_device);
        goto class_del;
    }

    pr_info("PCD module loaded successfully\n");

    return 0;
class_del:
    class_destroy(pcd_class);
cdev_del:
    cdev_del(&pcd_cdev);
unreg_chrdev:
    unregister_chrdev_region(device_number, 1);
out:
    pr_info("Module insertion failed\n");
    return ret;
}
    

static void __exit pcd_driver_exit(void)
{
    device_destroy(pcd_class, device_number);
    class_destroy(pcd_class);
    cdev_del(&pcd_cdev);
    unregister_chrdev_region(device_number, 1);
    
    pr_info("PCD Module Unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nhat Tran");
MODULE_DESCRIPTION("A pseudo character driver");