#ifndef _GOODIX_SECURE_TOUCH_H_
#define _GOODIX_SECURE_TOUCH_H_

#include <linux/stat.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/i2c-qcom-geni.h>
#include "goodix_ts_core.h"

extern struct goodix_secure_data *goodix_secure_data;

extern int goodix_secure_init(void);
extern void goodix_secure_touch_init(void);
extern void goodix_secure_touch_stop(int block);
extern irqreturn_t goodix_filter_interrupt(void);
extern irqreturn_t goodix_ts_work_thread(int irq, void *data);

struct goodix_secure_data {
     struct input_dev *input_dev;
     struct i2c_client *i2c_client;
	 struct mutex i2c_ctrl;
	 atomic_t st_enabled;
	 atomic_t st_pending_irqs;
	 bool st_initialized;
	 struct completion st_powerdown;
	 struct completion st_irq_processed;
	 struct goodix_ts_core *core_data ;
	 struct geni_i2c_dev *gi2c;
	 struct clk *se_clk;
	 struct clk *m_ahb_clk;
	 struct clk *s_ahb_clk;
};

#endif	//_GOODIX_SECURE_TOUCH_H_
