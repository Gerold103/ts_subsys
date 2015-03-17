#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/mutex.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <linux/cdev.h>

#include "ts_ops.h"

MODULE_LICENSE("Dual BSD/GPL");

#define TIMER_INTERVAL_SECS 1
#define FREQUENCY_VAL 1.5
#define ADDITIONAL_TIME_NS 50

//------------------------D E F I N I T I O N S------------------------

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart);

//------------------------R E A L I Z A T I O N S------------------------

struct timespec get_systime_from_devtime(struct ts_dev *dev, struct timespec dev_time)
{
	/*
	???????
	sys_time: 0  1  2  3  4  5 ...
	dev1:     x 2x 3x 4x 5x 6x ...
	dev2:           0  y 2y 3y  
	Then in 1 real second in dev passes x seconds. And when in dev time is X then real time is X / x ?

	sys_time = alpha * dev_time + coef;
	*/
	double alpha;
	struct timespec coef;
	struct timespec result;
	struct timespec buf;
	{
		struct timespec up = timespec_sub(dev->sys_ts1, dev->sys_ts2);
		struct timespec down = timespec_sub(dev->ts1, dev->ts2);
		alpha = (timespec_to_ns(&up) + 0.0) / (timespec_to_ns(&down));
	}
	coef = dev->sys_ts1;

	set_normalized_timespec(&buf, dev->ts1.tv_sec * alpha, dev->ts1.tv_nsec * alpha);

	coef = timespec_sub(coef, buf);

	set_normalized_timespec(&buf, dev_time.tv_sec * alpha, dev_time.tv_nsec * alpha);
	result = timespec_add(buf, coef);
	return result;
	//struct timespec x = timespec_sub(dev->ts1, dev->ts2);
	//return ns_to_timespec(((timespec_to_ns(&dev_time) + 0.0) / timespec_to_ns(&x)) * (dev->hint.ns_interval));
	//return current_kernel_time();
}

bool ts_register(struct ts_dev *dev)
{
	ktime_t interval = ktime_set(0, dev->hint.ns_interval + ADDITIONAL_TIME_NS);
	dev->ts1 = (*(dev->getcurtime))();
	dev->sys_ts1 = current_kernel_time();
	hrtimer_init(&(dev->hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dev->hr_timer.function = &timer_callback;
	hrtimer_start(&(dev->hr_timer), interval, HRTIMER_MODE_REL);
	return true;
}

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart) {
	ktime_t cur_time, interval;
	//Here we read new timestamp from every registered dev
	struct ts_dev *dev = container_of(timer_for_restart, struct ts_dev, hr_timer);

	dev->sys_ts2 = dev->sys_ts1;
	dev->ts2 = dev->ts1;

	dev->ts1 = (*(dev->getcurtime))();
	dev->sys_ts1 = current_kernel_time();

	cur_time = ktime_get();
	interval = ktime_set(0, dev->hint.ns_interval * 2 + ADDITIONAL_TIME_NS);
	printk(KERN_ALERT "timer_callback: ts1 secs = %d, ts2 secs = %d\n", (int)(dev->ts1.tv_sec), (int)(dev->ts2.tv_sec));
	hrtimer_forward(timer_for_restart, cur_time, interval);
	return HRTIMER_RESTART;
}

void ts_unregister(struct ts_dev *dev)
{
	hrtimer_cancel(&(dev->hr_timer));
}


EXPORT_SYMBOL(get_systime_from_devtime);
EXPORT_SYMBOL(ts_register);
EXPORT_SYMBOL(ts_unregister);

//------------------------M O D U L E   I N I T------------------------

static int __init ts_subsys_init(void)
{;
	printk(KERN_ALERT "TS_SUBSYS init done\n");
	return 0;
}

static void __exit ts_subsys_exit(void) {
	printk(KERN_ALERT "TS_SUBSYS terminated\n");
}

module_init(ts_subsys_init);
module_exit(ts_subsys_exit);
