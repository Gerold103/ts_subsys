#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/mutex.h>

#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/export.h>

#include <linux/cdev.h>

#include "/home/gerold103/Sciwork/headers/ts_ops.h"

MODULE_LICENSE("Dual BSD/GPL");

#define TIMER_INTERVAL_SECS 1

//------------------------D E F I N I T I O N S------------------------

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart);

EXPORT_SYMBOL(get_systime_from_devtime);
EXPORT_SYMBOL(ts_register);
EXPORT_SYMBOL(ts_unregister);

//------------------------R E A L I Z A T I O N S------------------------

uint get_systime_from_devtime(struct ts_dev *dev, uint dev_time)
{
	/*
	???????
	sys_time: 0  1  2  3  4  5 ...
	dev1:     0  x 2x 3x 4x 5x ...
	dev2:           0  y 2y 3y  
	Then in 1 real second in dev passes x seconds. And when in dev time is X then real time is X / x ?
	*/
	uint x = dev->ts1 - dev->ts2;
	return dev_time / x;
}

bool ts_register(struct ts_dev *dev)
{
	ktime_t interval = ktime_set(TIMER_INTERVAL_SECS, 0);
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
