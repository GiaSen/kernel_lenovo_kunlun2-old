#include <linux/errno.h>
#include <linux/atomic.h>
#include <linux/pm_runtime.h>
#include <linux/qcom-geni-se.h>
#include <linux/i2c-qcom-geni.h>
#include <linux/clk-provider.h>

#include "goodix_secure_touch.h"
#include "goodix_ts_core.h"

#define TZ    1
#define	APSS  3
#define TZ_BLSP_MODIFY_OWNERSHIP_ID 3
//#define reinit_completion init_completion

struct clk;

extern irqreturn_t goodix_ts_threadirq_func(int irq, void *data);
static ssize_t goodix_secure_touch_enable_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf);

static ssize_t goodix_secure_touch_enable_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count);

static ssize_t goodix_secure_touch_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf);

void goodix_secure_touch_notify(void);


struct goodix_secure_data *goodix_secure_data;

int goodix_clk_prepare_enable(void)
{
	int ret;

	ret = clk_prepare_enable(goodix_secure_data->m_ahb_clk);
	if (ret) {
		clk_disable_unprepare(goodix_secure_data->se_clk);
		ts_err("error on clk_prepare_enable(m_ahb_clk):%d\n", ret);
        return ret;
	}
	ret = clk_prepare_enable(goodix_secure_data->s_ahb_clk);
	if (ret) {
		clk_disable_unprepare(goodix_secure_data->m_ahb_clk);
		ts_err("error on clk_prepare_enable(s_ahb_clk):%d\n", ret);
        return ret;
	}

    ret = clk_prepare_enable(goodix_secure_data->se_clk);
	if (ret) {
		clk_disable_unprepare(goodix_secure_data->s_ahb_clk);
		clk_disable_unprepare(goodix_secure_data->m_ahb_clk);
		ts_err("error on clk_prepare_enable(se_clk):%d\n", ret);
		return ret;
	}

	return ret;
}

void goodix_clk_disable_unprepare(void)
{
    ts_info("disable clk\n");
	clk_disable_unprepare(goodix_secure_data->s_ahb_clk);
	clk_disable_unprepare(goodix_secure_data->m_ahb_clk);
	clk_disable_unprepare(goodix_secure_data->se_clk);
}

int goodix_i2c_get(void)
{
	int retval;
	struct i2c_client *i2c =goodix_secure_data->i2c_client;

	mutex_lock(&goodix_secure_data->i2c_ctrl);
	retval = pm_runtime_get_sync(i2c->adapter->dev.parent);
	if (retval >= 0) {
		retval = goodix_clk_prepare_enable();
		if (retval)
			pm_runtime_put_sync(i2c->adapter->dev.parent);
	}
	mutex_unlock(&goodix_secure_data->i2c_ctrl);
	return retval;
}

void goodix_i2c_put(void)
{
	struct i2c_client *i2c = goodix_secure_data->i2c_client;

	mutex_lock(&goodix_secure_data->i2c_ctrl);
	goodix_clk_disable_unprepare();
	pm_runtime_put_sync(i2c->adapter->dev.parent);
	mutex_unlock(&goodix_secure_data->i2c_ctrl);
}

static struct kobj_attribute tz_attrs[] = {
	__ATTR(secure_touch_enable, (S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP),
            goodix_secure_touch_enable_show, goodix_secure_touch_enable_store),
	__ATTR(secure_touch, (S_IRUSR | S_IRGRP | S_IROTH), goodix_secure_touch_show, NULL)
};

static ssize_t goodix_secure_touch_enable_show(
        struct kobject *kobj,
        struct kobj_attribute *attr,
        char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d",
					atomic_read(&goodix_secure_data->st_enabled));
}

static ssize_t goodix_secure_touch_enable_store(
        struct kobject *kobj,
        struct kobj_attribute *attr,
        const char *buf,
        size_t count)
{
	unsigned long value;
	int err = 0;

	if (count > 2){
		return -EINVAL;
    }

	err = kstrtoul(buf, 10, &value);
	if (err != 0){
        ts_err("%s err : 0x%x\n", __func__, err );
		return err;
    }

	ts_debug("Secure Touch Enable Store value is %ld", value);
	if (!goodix_secure_data->st_initialized) {
		ts_err("secure touch not initialized!");
		return -EIO;
	}

	err = count;
	switch (value) {
	case 0:
		if (atomic_read(&goodix_secure_data->st_enabled) == 0)
			break;
		ts_info("Secure Touch Enable Store switch OFF");
		goodix_i2c_put();
		atomic_set(&goodix_secure_data->st_enabled, 0);
		goodix_secure_touch_notify();
		complete(&goodix_secure_data->st_irq_processed);
		goodix_ts_threadirq_func(goodix_secure_data->i2c_client->irq, goodix_secure_data->core_data);
		complete(&goodix_secure_data->st_powerdown);
		break;
	case 1:
		if (atomic_read(&goodix_secure_data->st_enabled)) {
			err = -EBUSY;
			break;
		}
		ts_info("Secure Touch Enable Store switch ON");
		synchronize_irq(goodix_secure_data->i2c_client->irq);
		if (goodix_i2c_get() < 0) {
			ts_err("goodix_i2c_get failed");
			err = -EIO;
			break;
		}
		ts_info("Secure Touch Enable Store Change Pipe Owner");
		reinit_completion(&goodix_secure_data->st_powerdown);
		reinit_completion(&goodix_secure_data->st_irq_processed);
		atomic_set(&goodix_secure_data->st_enabled, 1);
		atomic_set(&goodix_secure_data->st_pending_irqs,  0);
		break;
	default:
		ts_err("unsupported value: %lu\n", value);
		err = -EINVAL;
		break;
	}
	return err;
}

static ssize_t goodix_secure_touch_show(
        struct kobject *kobj,
        struct kobj_attribute *attr,
        char *buf)
{
	int val = 0;

	if (atomic_read(&goodix_secure_data->st_enabled) == 0){
		ts_info("Secure Touch Show:goodix_secure_data st_enabled is 0");
		return -EBADF;
	}

	if (atomic_cmpxchg(&goodix_secure_data->st_pending_irqs, -1, 0) == -1){
		ts_info("Secure Touch Show:st_pending_irqs is -1");
		atomic_set(&goodix_secure_data->st_pending_irqs, -1);
		return -EINVAL;
	}

	if (atomic_cmpxchg(&goodix_secure_data->st_pending_irqs, 1, 0) == 1){
		ts_info("Secure Touch Show:goodix_secure_data st_pending_irqs is 1");
		val = 1;
	}else{
		complete(&goodix_secure_data->st_irq_processed);
	}
	return scnprintf(buf, PAGE_SIZE, "%u", val);
}

int goodix_secure_init(void)
{
	int err, i;
	struct i2c_client *client = goodix_secure_data->i2c_client;

	goodix_secure_data->input_dev = input_allocate_device();
	if(!goodix_secure_data->input_dev) {
		ts_err("Failed to alloc input device for tz!");
		err = -ENOMEM;
		goto err_exit;
	}

	goodix_secure_data->input_dev->name = "goodix_secure";
	goodix_secure_data->input_dev->phys = "goodix/secure";
	goodix_secure_data->input_dev->id.bustype = BUS_I2C;

	set_bit(EV_ABS, goodix_secure_data->input_dev->evbit);

	err = input_register_device(goodix_secure_data->input_dev);
	if(err) {
		ts_err("Failed to register tz input device: %s!", goodix_secure_data->input_dev->name);
		goto err_exit;
	}

	goodix_secure_data->input_dev->dev.parent = &client->dev;
	//	create sysfs files
	for(i = 0; i < sizeof(tz_attrs)/sizeof(tz_attrs[0]); i++) {
		if((err = sysfs_create_file(&(goodix_secure_data->i2c_client)->dev.kobj, &tz_attrs[i].attr))) {
			goto err_exit;
		}
	}

	ts_info("secure devices init OK.");
	return 0;

err_exit:
	return err;
}

void goodix_secure_touch_init(void)
{

	goodix_secure_data->st_initialized = 0;
	init_completion(&goodix_secure_data->st_powerdown);
	init_completion(&goodix_secure_data->st_irq_processed);

#if 1
    ts_info("i2c_client->name : %s\n", goodix_secure_data->i2c_client->adapter->name);
    ts_info("i2c_client->name : %s\n", goodix_secure_data->i2c_client->name);
    goodix_secure_data->gi2c = i2c_get_adapdata( goodix_secure_data->i2c_client->adapter );
	/* Get clocks */
    if( NULL == goodix_secure_data->gi2c ){
        ts_err(" get gi2c failed\n");
        return;
    }
	goodix_secure_data->se_clk = goodix_secure_data->gi2c->i2c_rsc.se_clk;
	goodix_secure_data->m_ahb_clk = goodix_secure_data->gi2c->i2c_rsc.m_ahb_clk;
	goodix_secure_data->s_ahb_clk = goodix_secure_data->gi2c->i2c_rsc.s_ahb_clk;
#endif

    mutex_init(&goodix_secure_data->i2c_ctrl);
    goodix_secure_data->st_initialized = 1;

    return;
}

void goodix_secure_touch_notify(void)
{
    sysfs_notify(&goodix_secure_data->i2c_client->dev.kobj, NULL, "secure_touch");
}

irqreturn_t goodix_filter_interrupt(void)
{
    if (atomic_read(&goodix_secure_data->st_enabled)) {
        if (atomic_cmpxchg(&goodix_secure_data->st_pending_irqs, 0, 1) == 0) {
            reinit_completion(&goodix_secure_data->st_irq_processed);
            goodix_secure_touch_notify();
            wait_for_completion_interruptible(&goodix_secure_data->st_irq_processed);
        }
        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

void goodix_secure_touch_stop(int blocking)
{
    ts_debug("goodix_secure_touch_stop start...");
    if (atomic_read(&goodix_secure_data->st_enabled)) {
        atomic_set(&goodix_secure_data->st_pending_irqs, -1);
        goodix_secure_touch_notify();
        if (blocking) {
            wait_for_completion_interruptible(&goodix_secure_data->st_powerdown);
        }
    }
}


