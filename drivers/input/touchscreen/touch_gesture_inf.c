#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>

struct tp_sys_info{
        struct class *tp_class;
        int index;
        struct device *dev;
};
static atomic_t tp_device_count;
//int gesture_dubbleclick_en = 0;
static int double_gesture_switch;
#ifdef CONFIG_PRODUCT_ZAP
extern void gsx_gesture_en(int enable);
#endif
//add by gongdb begin
static int tp_screen_orientation_switch = 0;
//add by gongdb end

#if defined(CONFIG_PRODUCT_JD2019) || defined(CONFIG_PRODUCT_KUNLUN2)
extern void fts_gesture_en(int enable);

//add by gongdb degin
extern void fts_tp_screen_switch(int enable);
//add by gongdb end
#endif
#ifdef CONFIG_PRODUCT_KUNLUN
extern void synaptics_gesture_en(int enable);
#endif
#ifdef CONFIG_PRODUCT_SPROUT
extern void nvt_gesture_en(int enable);
#endif
static ssize_t ft_gesture_wakeup_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n",double_gesture_switch);
}

static ssize_t ft_gesture_wakeup_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
       int val;

       sscanf(buf, "%d", &val);
	if(val)
		double_gesture_switch = 1;
	else
		double_gesture_switch = 0;
#if defined(CONFIG_PRODUCT_JD2019) || defined(CONFIG_PRODUCT_KUNLUN2)
	fts_gesture_en(double_gesture_switch);
#endif
#ifdef CONFIG_PRODUCT_KUNLUN
	synaptics_gesture_en(double_gesture_switch);
#endif
#if defined(CONFIG_PRODUCT_SPROUT)
	nvt_gesture_en(double_gesture_switch);
#endif
#ifdef CONFIG_PRODUCT_ZAP
	gsx_gesture_en(double_gesture_switch);
#endif
       return count;

}
#if 0
int is_gesture_dubbleclick_en(void)
{
	return gesture_dubbleclick_en;
}
EXPORT_SYMBOL_GPL(is_gesture_dubbleclick_en);
#endif

//add by gongdb begin
static ssize_t tp_screen_orientation_switch_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%d\n", tp_screen_orientation_switch);
}

static ssize_t tp_screen_orientation_switch_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
       int val;

	sscanf(buf, "%d", &val);

	//val 0 : 0 degree, 1 : 90 degree, 2 : 180 degree, 3 : 270 degree
        if((val == 1) || (val == 3))
                tp_screen_orientation_switch = 1;
        else
                tp_screen_orientation_switch = 0;

#if defined(CONFIG_PRODUCT_JD2019) || defined(CONFIG_PRODUCT_KUNLUN2)
        fts_tp_screen_switch(tp_screen_orientation_switch);
#endif
       return count;

}

#if defined(CONFIG_PRODUCT_JD2019) || defined(CONFIG_PRODUCT_KUNLUN2)
void fts_reset_tp_screen_switch(void)
{
	fts_tp_screen_switch(tp_screen_orientation_switch);
}
#endif
//add by gongdb end

static struct device_attribute attrs[] = {
        __ATTR(gesture_on, 0664,
                        ft_gesture_wakeup_show,
                        ft_gesture_wakeup_store),
//add by gongdb begin
	__ATTR(tp_screen_switch, 0664,
                        tp_screen_orientation_switch_show,
                        tp_screen_orientation_switch_store),
//add by gongdb end
};

static int tp_gesture_ctl_class(void)
{
       int attr_count = 0;
	int err;
 	struct tp_sys_info *ts;

       ts = kzalloc(sizeof(*ts), GFP_KERNEL);
       memset(ts, 0, sizeof(*ts));
       ts->tp_class = class_create(THIS_MODULE, "touch");
       if (IS_ERR(ts->tp_class))
       {
       	printk("create tp class err!");
	}
	else
     	       atomic_set(&tp_device_count, 0);

	ts->index = atomic_inc_return(&tp_device_count);
       ts->dev = device_create(ts->tp_class, NULL,
       MKDEV(0, ts->index), NULL, "tp_dev");
	if (IS_ERR(ts->dev))
	{
              printk("create device err!");
	}
       for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
              err = sysfs_create_file(&ts->dev->kobj,
                              &attrs[attr_count].attr);
              if (err < 0) {
                   pr_err("%s: Failed to create sysfs attributes\n", __func__);
                   return err;
             }
        }
        dev_set_drvdata(ts->dev,ts);
        //end tp class to show tp info

	return 0;
}

static int __init tp_gesture_ctl_init(void)
{
	return tp_gesture_ctl_class();
}

static void __exit tp_gesture_ctl_exit(void)
{
	//if (ts & ts->tp_class)
	//	class_destroy(ts->tp_class);
}

module_init(tp_gesture_ctl_init);
module_exit(tp_gesture_ctl_exit);

MODULE_AUTHOR("xxx <xxx@android.com>");
MODULE_DESCRIPTION("tp gesture control class driver");
MODULE_LICENSE("GPL");
