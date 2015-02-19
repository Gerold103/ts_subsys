typedef unsigned int uint;

//Struct provides pointer on dev, which was registered before, and two last timestamps, represented dev time.
//They was read in interval TIMER_INTERVAL_SECS in terms of this module. Registered module must implement function getcurtime.
struct ts_dev {
	const char *module_name;
	struct timespec (*getcurtime)(void);
	struct hrtimer hr_timer;
	struct timespec ts1;
	struct timespec ts2;
};

//This function registered devs will call for reduce their time to real time
extern struct timespec get_systime_from_devtime(struct ts_dev *dev, struct timespec dev_time);

//This function each dev must call for register self in subsystem
extern bool ts_register(struct ts_dev *dev);

//This function each dev must call, when it terminates
extern void ts_unregister(struct ts_dev *dev);