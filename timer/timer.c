#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <linux/cdev.h>

#include "/home/gerold103/Sciwork/headers/ts_ops.h"

MODULE_LICENSE("Dual BSD/GPL");

#define TIMER_INTERVAL_SECS 1

//------------------------D E F I N I T I O N S------------------------

//Struct, describing this device
struct sdtimer_dev {
	size_t value;

	size_t vbuf_size;
	char *vbuf;
	
	struct hrtimer hr_timer;
	
	struct cdev cdev;
};

struct sdtimer_dev sdt_dev;

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart);

int sdtimer_open(struct inode *inode, struct file *filp);

int sdtimer_release(struct inode *inode, struct file *filp);

long sdtimer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t sdtimer_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

extern uint get_systime_from_devtime(struct ts_dev *dev, uint dev_time);
extern bool ts_register(struct ts_dev *dev);
extern void ts_unregister(struct ts_dev *dev);

struct file_operations sdt_fops = {
	.owner = THIS_MODULE,
	.read = sdtimer_read,
	.unlocked_ioctl = sdtimer_ioctl,
	.open = sdtimer_open,
	.release = sdtimer_release,
};

dev_t devno;

uint get_current_time(void);

struct ts_dev tsdev = {
	.module_name = THIS_MODULE->name,
	.getcurtime = get_current_time,
};

//------------------------R E A L I Z A T I O N S------------------------

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart) {
	ktime_t cur_time, interval;
	
	//hr_timer restart
	cur_time = ktime_get();
	interval = ktime_set(TIMER_INTERVAL_SECS, 0);
	hrtimer_forward(timer_for_restart, cur_time, interval);
	
	sdt_dev.value++;
	
	return HRTIMER_RESTART;
}

int sdtimer_open(struct inode *inode, struct file *filp) {
	struct sdtimer_dev *dev;
	unsigned int  value, vbuf_size, vbuf_p;
	
	dev = container_of(inode->i_cdev, struct sdtimer_dev, cdev);
	filp->private_data = dev;
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		printk(KERN_ALERT "Invalid open flag: sdtimer_dev is read only\n");
		return -1;
	}
	
	value = dev->value, vbuf_size = 0;
	if (!value)
		vbuf_size = 1;
	while (value > 0) {
		value /= 10;
		vbuf_size++;
	}
	
	dev->vbuf_size = vbuf_size;
	dev->vbuf = kmalloc(vbuf_size + 1, GFP_KERNEL);
	if (dev->vbuf == NULL) {
		printk(KERN_ALERT "Not enough memory for dev buffer\n");
		return -1;
	}
	value = dev->value;
	if (!value)
		dev->vbuf[0] = '0', dev->vbuf[1] = '\0';
	vbuf_p = vbuf_size;
	dev->vbuf[vbuf_p--] = '\0';
	while (value > 0) {
		dev->vbuf[vbuf_p--] = value % 10 + '0';
		value /= 10;
	}
	
	return 0;
}

int sdtimer_release(struct inode *inode, struct file *filp) {
	struct sdtimer_dev *dev;
	
	dev = container_of(inode->i_cdev, struct sdtimer_dev, cdev);
	filp->private_data = dev;
	
	//release vbuf memory
	kfree(dev->vbuf);
	dev->vbuf_size = 0;
	
	return 0;
}

long sdtimer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	return 0;
}

ssize_t sdtimer_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	struct sdtimer_dev *dev;
	int cur_time = get_systime_from_devtime(&tsdev, sdt_dev.value);

	dev = filp->private_data;
	
	if (*f_pos < dev->vbuf_size) {
		int i;
		for (i = 0; (i < count) && (i + *f_pos < dev->vbuf_size); i++) {
			buf[i] = dev->vbuf[i + *f_pos];
		}
		*f_pos += i;
		return i;
	}

	printk(KERN_INFO "Current sys time: %d\n", cur_time);
	return 0;
}

uint get_current_time(void)
{
	return (uint)(sdt_dev.value);
}

//------------------------M O D U L E   I N I T------------------------

static int __init timer_init(void) {
	int err;
	ktime_t interval;
	
	//register dev
	err = alloc_chrdev_region(&devno, 0, 1, "sd_timer");
	if (err) {
		printk(KERN_ALERT "Cannot register device. Module terminating\n");
		return -1;
	}
	
	cdev_init(&(sdt_dev.cdev), &sdt_fops);
	
	sdt_dev.cdev.owner = THIS_MODULE;
	sdt_dev.cdev.ops = &sdt_fops;
	
	//init hr_timer
	sdt_dev.value = 0;
	
	interval = ktime_set(TIMER_INTERVAL_SECS, 0);
	hrtimer_init(&sdt_dev.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sdt_dev.hr_timer.function = &timer_callback;
	hrtimer_start(&sdt_dev.hr_timer, interval, HRTIMER_MODE_REL);
	
	err = cdev_add(&sdt_dev.cdev, devno, 1);
	if (err)
		printk(KERN_ALERT "Error %d adding timer module\n", err);

	//Here we register our device in ts_subsys

	ts_register(&tsdev);
	
	printk(KERN_ALERT "Module init done\n");
	return 0;
}

static void __exit timer_exit(void) {
	hrtimer_cancel(&sdt_dev.hr_timer);
	ts_unregister(&tsdev);
	//unregister dev
	unregister_chrdev_region(devno, 1);
	
	//work done
	printk(KERN_ALERT "Module terminated\n");
}

module_init(timer_init);
module_exit(timer_exit);