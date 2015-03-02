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
	*/
	uint x = dev->ts1 - dev->ts2;
	if (x) return dev_time / x;
	else return current_kernel_time();
	//return current_kernel_time();
}

bool ts_register(struct ts_dev *dev)
{
	//ktime_t interval = ktime_set(TIMER_INTERVAL_SECS, 0);
	ktime_t interval = timespec_to_ktime(dev->hint.frequency);
	interval = ktime_set(0, ktime_to_ns(interval) * FREQUENCY_VAL);
	dev->ts1 = (*(dev->getcurtime))();
	hrtimer_init(&(dev->hr_timer), CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dev->hr_timer.function = &timer_callback;
	hrtimer_start(&(dev->hr_timer), interval, HRTIMER_MODE_REL);
	return true;
}

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart) {
	ktime_t cur_time, interval;
	//Here we read new timestamp from every registered dev
	struct ts_dev *dev = container_of(timer_for_restart, struct ts_dev, hr_timer);
	dev->ts2 = dev->ts1;
	dev->ts1 = (*(dev->getcurtime))();
	cur_time = ktime_get();
	interval = ktime_set(TIMER_INTERVAL_SECS, 0);
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
