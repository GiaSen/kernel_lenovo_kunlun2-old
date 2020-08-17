#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <video/mmp_disp.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>


extern int FL_Enable(void);
extern int FL_Disable(void);
static struct kobject *bootinfo_kobj = NULL;

extern char front_cam_name[64];
extern char frontaux_cam_name[64];
extern char back_cam_name[64];
extern char backaux_cam_name[64];

extern char front_cam_efuse_id[64];
extern char frontaux_cam_efuse_id[64];
extern char back_cam_efuse_id[64];
extern char backaux_cam_efuse_id[64];



//static u8 lcd_array[]="lcd not found!";

int gesture_dubbleclick_en =0;
extern bool tp_probe_ok;//bit0
//bool camera_front_probe_ok;//bit1
//bool camera_back_probe_ok;//bit2
extern bool gsensor_probe_ok;//bit3
extern bool proximity_probe_ok;//bit4
extern bool charger_probe_ok;//bit5
//extern bool pmu_probe_ok=1;//bit6
extern bool compass_probe_ok;//bit7
extern bool fingerprint_probe_ok;//bit31
//extern int sboot_efuse_info_attr;
static ssize_t gesture_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	return (s - buf);
}

static ssize_t gesture_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)

{
    int enable=0,ret;
    ret = kstrtouint(buf, 10, &enable);
	if(ret != 0)
		return 0;
    gesture_dubbleclick_en =enable;

	return n;
}
static struct kobj_attribute gesture_enable_attr = {
	.attr = {
		.name = "gesture_enable",
		.mode = 0644,
	},
	.show =&gesture_enable_show,
	.store= &gesture_enable_store,
};

static ssize_t back_camera_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

		s += sprintf(s, "%s\n",back_cam_name);

	return (s - buf);
}

static ssize_t back_camera_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}

static struct kobj_attribute back_camera_info_attr = {
	.attr = {
		.name = "back_camera",
		.mode = 0644,
	},
	.show =&back_camera_info_show,
	.store= &back_camera_info_store,
};

static ssize_t backaux_camera_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",backaux_cam_name);

	return (s - buf);
}

static ssize_t backaux_camera_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute backaux_camera_info_attr = {
	.attr = {
		.name = "backaux_camera",
		.mode = 0644,
	},
	.show =&backaux_camera_info_show,
	.store= &backaux_camera_info_store,
};
static ssize_t front_camera_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",front_cam_name);

	return (s - buf);
}

static ssize_t front_camera_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute front_camera_info_attr = {
	.attr = {
		.name = "front_camera",
		.mode = 0644,
	},
	.show =&front_camera_info_show,
	.store= &front_camera_info_store,
};

static ssize_t frontaux_camera_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",frontaux_cam_name);

	return (s - buf);
}

static ssize_t frontaux_camera_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute frontaux_camera_info_attr = {
	.attr = {
		.name = "frontaux_camera",
		.mode = 0644,
	},
	.show =&frontaux_camera_info_show,
	.store= &frontaux_camera_info_store,
};



static ssize_t back_camera_id_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

		s += sprintf(s, "%s\n",back_cam_efuse_id);

	return (s - buf);
}

static ssize_t back_camera_id_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}

static struct kobj_attribute back_camera_id_attr = {
	.attr = {
		.name = "back_camera_id",
		.mode = 0644,
	},
	.show =&back_camera_id_show,
	.store= &back_camera_id_store,
};

static ssize_t backaux_camera_id_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",backaux_cam_efuse_id);

	return (s - buf);
}

static ssize_t backaux_camera_id_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute backaux_camera_id_attr = {
	.attr = {
		.name = "backaux_camera_id",
		.mode = 0644,
	},
	.show =&backaux_camera_id_show,
	.store= &backaux_camera_id_store,
};

static ssize_t front_camera_id_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",front_cam_efuse_id);

	return (s - buf);
}

static ssize_t front_camera_id_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute front_camera_id_attr = {
	.attr = {
		.name = "front_camera_id",
		.mode = 0644,
	},
	.show =&front_camera_id_show,
	.store= &front_camera_id_store,
};

 static ssize_t frontaux_camera_id_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

	s += sprintf(s, "%s\n",frontaux_cam_efuse_id);

	return (s - buf);
}

static ssize_t frontaux_camera_id_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute frontaux_camera_id_attr = {
	.attr = {
		.name = "frontaux_camera_id",
		.mode = 0644,
	},
	.show =&frontaux_camera_id_show,
	.store= &frontaux_camera_id_store,
};



//gps for reset and update
#define GPS_UPDATE   1
#define GPS_RESET    8
#define GPS_AVDD_EN  0
#define GPS_PRTRG   40

int v_atoi(char* str)
{
	bool bMinus = false;
	int result = 0;
	char* p = str;
	if (!p)
		return -1;

    while(*p == ' ') p++;

	if (('0' > *p || *p > '9') && (*p == '+' || *p == '-'))
	{
		if (*p++ == '-')
			bMinus = true;
	}

	while ( *p != '\n')
	{
		if ('0' > *p || *p > '9')
            break;
		else
			result = result * 10 + (*p++ - '0');
	}

	if (*p != '\n')//no-normal end
		return -2;

	return bMinus ? -result : result;
}

static ssize_t gps_reset_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	int ret = 0;
	if (!gpio_is_valid(GPS_RESET)) {

		ret = gpio_request(GPS_RESET, "gps_reset");
		if (ret) {
			pr_err("%s:Failed to request GPS RESET GPIO, ret = %d\n", __func__, ret);
		}
	}
	gpio_direction_input(GPS_RESET);
	s += sprintf(s, "%d\n", gpio_get_value(GPS_RESET));
	return (s - buf);
}

static ssize_t gps_reset_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	char s[64] = {0};
	int v = 0;
	int ret = 0;
	if (n > 0 && n < 64)
	{
		memcpy(s, buf, n);
		v = v_atoi(s);
		if (!gpio_is_valid(GPS_RESET)) {

			ret = gpio_request(GPS_RESET, "gps_reset");
			if (ret) {
				pr_err("%s:Failed to request GPS RESET GPIO, ret = %d\n", __func__, ret);
				//goto fail;
			}
		}
		gpio_direction_output(GPS_RESET, 0);
		gpio_set_value(GPS_RESET, (v == 0) ? 0 : 1);
		pr_err("%s:set reset is %d \n", __func__,v);
	}
	return n;
}

static struct kobj_attribute gps_reset_attr = {
	.attr = {
		.name = "gps_reset",
		.mode = 0644,
	},
	.show =&gps_reset_show,
	.store= &gps_reset_store,
};

static ssize_t gps_update_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	int ret = 0;
	if (!gpio_is_valid(GPS_UPDATE)) {

		ret = gpio_request(GPS_UPDATE, "gps_update");
		if (ret) {
			pr_err("%s:Failed to request GPS UPDATA GPIO, ret = %d\n", __func__, ret);
		}
	}
	gpio_direction_input(GPS_UPDATE);
	s += sprintf(s, "%d\n", gpio_get_value(GPS_UPDATE));

	return (s - buf);
}

static ssize_t gps_update_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	char s[64] = {0};
	int v = 0;
	int ret = 0;
	if (n > 0 && n < 64)
	{
		memcpy(s, buf, n);
		v = v_atoi(s);
		if (!gpio_is_valid(GPS_UPDATE)) {

			ret = gpio_request(GPS_UPDATE, "gps_update");
			if (ret) {
				pr_err("%s:Failed to request GPS UPDATE GPIO, ret = %d\n", __func__, ret);
				//goto fail;
			}
		}
		gpio_direction_output(GPS_UPDATE, 1);
		gpio_set_value(GPS_UPDATE, (v == 0) ? 0 : 1);
		pr_err("%s:set update is %d \n", __func__,v);
	}
	return n;
}

static struct kobj_attribute gps_update_attr = {
	.attr = {
		.name = "gps_lna",
		.mode = 0644,
	},
	.show =&gps_update_show,
	.store= &gps_update_store,
};

static ssize_t gps_avdd_en_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	//s += sprintf(s, "%s\n",mtkfb_find_lcm_driver());
	return (s - buf);
}

static ssize_t gps_avdd_en_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	char s[64] = {0};
	int v = 0;
	int ret = 0;
	if (n > 0 && n < 64)
	{
		memcpy(s, buf, n);
		v = v_atoi(s);
		if (!gpio_is_valid(GPS_AVDD_EN)) {

			ret = gpio_request(GPS_AVDD_EN, "gps_avdd_en");
			if (ret) {
				pr_err("%s:Failed to request GPS AVDD EN GPIO, ret = %d\n", __func__, ret);
				//goto fail;
			}
		}
		gpio_direction_output(GPS_AVDD_EN, 1);
		gpio_set_value(GPS_AVDD_EN, (v == 0) ? 0 : 1);
		pr_err("%s:set avdd en is %d \n", __func__,v);
	}
	return n;
}

static struct kobj_attribute gps_avdd_en_attr = {
	.attr = {
		.name = "gps_avdd_en",
		.mode = 0644,
	},
	.show =&gps_avdd_en_show,
	.store= &gps_avdd_en_store,
};

static ssize_t gps_prtrg_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	int ret = 0;
	if (!gpio_is_valid(GPS_PRTRG)) {

		ret = gpio_request(GPS_PRTRG, "gps_prtrg");
		if (ret) {
			pr_err("%s:Failed to request GPS PRTRG GPIO, ret = %d\n", __func__, ret);
		}
	}
	gpio_direction_input(GPS_PRTRG);
	s += sprintf(s, "%d\n", gpio_get_value(GPS_PRTRG));

	return (s - buf);
}

static ssize_t gps_prtrg_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	char s[64] = {0};
	int v = 0;
	int ret = 0;
	if (n > 0 && n < 64)
	{
		memcpy(s, buf, n);
		v = v_atoi(s);
		if (!gpio_is_valid(GPS_PRTRG)) {

			ret = gpio_request(GPS_PRTRG, "gps_prtrg");
			if (ret) {
				pr_err("%s:Failed to request GPS PRTRG GPIO, ret = %d\n", __func__, ret);
				//goto fail;
			}
		}
		gpio_direction_output(GPS_PRTRG, 1);
		gpio_set_value(GPS_PRTRG, (v == 0) ? 0 : 1);
		pr_err("%s:set gpio14 is %d \n", __func__,v);
	}
	return n;
}

static struct kobj_attribute gps_prtrg_attr = {
	.attr = {
		.name = "gps_prtrg",
		.mode = 0644,
	},
	.show =&gps_prtrg_show,
	.store= &gps_prtrg_store,
};

#if 0
extern char *mtkfb_find_lcm_driver(void);
static ssize_t lcd_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	s += sprintf(s, "%s\n",mtkfb_find_lcm_driver());
	return (s - buf);
}

static ssize_t lcd_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
static struct kobj_attribute lcd_info_attr = {
	.attr = {
		.name = "lcd_info",
		.mode = 0644,
	},
	.show =&lcd_info_show,
	.store= &lcd_info_store,
};
extern int sec_schip_enabled(void);
static ssize_t efuse_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	return sprintf(buf, "%d\n", sec_schip_enabled()? 1:0);
}

static struct kobj_attribute sboot_efuse_info_attr = {
	.attr = {
		.name = "hw_efuse_info",
		.mode = 0444,
	},
	.show =&efuse_info_show,
};
static ssize_t touch_flash_onoff_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	return sprintf(buf, "%d\n", touch_flash_enable ? 1:0);
}
static ssize_t touch_flash_onoff_info_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{

    int res=0;
    res = kstrtouint(buf, 10, &touch_flash_enable);

    if(touch_flash_enable==1)
    {
                FL_Enable();
    }
    else
    {
                FL_Disable();
    }
	return n;
}
static struct kobj_attribute touch_flash_onoff_info_attr = {
	.attr = {
		.name = "touch_flash",
		.mode = 0644,
	},
	.show =&touch_flash_onoff_info_show,
	.store= &touch_flash_onoff_info_store,
};
#endif

#if 0
static struct kobj_attribute modem_info_attr = {
	.attr = {
		.name = "modem_info",
		.mode = 0444,
	},
	.show = &modem_show,
};
#endif
//end
#if 0
static ssize_t lcd_driving_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
       unsigned int val;
       int res=0;

	 res = kstrtouint(buf, 10, &val);

	kernel_restart("nff_test");
	//lcm_kernel_detect_drv->esd_check();
	//if(lcd_detect_mipi_info.lcd_set_driving_mode)
	//	lcd_detect_mipi_info.lcd_set_driving_mode(&lcd_detect_mipi_info,val);
	//else
		//printk(KERN_ERR "[kernel]:lcd_set_driving_mode not found!.\n");
	return n;
}

static struct kobj_attribute lcd_driving_mode_set_attr = {
	.attr = {
		.name = "lcd_driving_mode_set_info",
		.mode = 0644,
	},
	.store = &lcd_driving_mode_store,
};
#endif
#if 1
static ssize_t i2c_devices_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
//	u8 string[5]={'\0'};
	int tmp=0;
	tmp|= (tp_probe_ok<<0);
	//tmp|= (camera_front_probe_ok<<1);
	//tmp|= (camera_back_probe_ok<<2);
	tmp|= (gsensor_probe_ok<<3);
	tmp|= (proximity_probe_ok<<4);
	tmp|= (charger_probe_ok<<5);
	//tmp|= (pmu_probe_ok<<6);
	tmp|= (compass_probe_ok<<7);
	tmp|= (fingerprint_probe_ok<<31);
	//itoa((int)tmp,string);
	s += sprintf(s, "0x%x\n",tmp);

	return (s - buf);
}
static struct kobj_attribute i2c_devices_info_attr = {
	.attr = {
		.name = "i2c_devices_probe_info",
		.mode = 0444,
	},
	.show =&i2c_devices_info_show,
};
#endif
//+add by hzb

#if 0
#if  defined (CONFIG_ARM64)  //Titan_TL PRJ
int get_pa_num(void)
{
     //add by pare for modem gpio check
#define GPIO_VERSION_PIN1    (GPIO95 | 0x80000000)
#define GPIO_VERSION_PIN2    (GPIO96 | 0x80000000)
#define GPIO_VERSION_PIN3    (GPIO93 | 0x80000000)
#define GPIO_VERSION_PIN4    (GPIO94 | 0x80000000)

	int pin1_val = 0, pin2_val = 0, pin3_val = 0, pin4_val = 0;

      mt_set_gpio_pull_select(GPIO_VERSION_PIN1, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN1, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_VERSION_PIN1, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_VERSION_PIN1, GPIO_DIR_IN);

      mt_set_gpio_pull_select(GPIO_VERSION_PIN2, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN2, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_VERSION_PIN2, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_VERSION_PIN2, GPIO_DIR_IN);

      mt_set_gpio_pull_select(GPIO_VERSION_PIN3, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN3, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_VERSION_PIN3, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_VERSION_PIN3, GPIO_DIR_IN);

      mt_set_gpio_pull_select(GPIO_VERSION_PIN4, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN4, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_VERSION_PIN4, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_VERSION_PIN4, GPIO_DIR_IN);

	mdelay(20);

	pin1_val = mt_get_gpio_in(GPIO_VERSION_PIN1);
	pin2_val = mt_get_gpio_in(GPIO_VERSION_PIN2);
	pin3_val = mt_get_gpio_in(GPIO_VERSION_PIN3);
	pin4_val = mt_get_gpio_in(GPIO_VERSION_PIN4);

	printk(KERN_ERR "%s:  pin1 is %d, pin2 is %d, pin3 is %d, pin4 is %d\n",__func__, pin1_val, pin2_val,pin3_val,pin4_val);

      mt_set_gpio_pull_enable(GPIO_VERSION_PIN1, GPIO_PULL_DISABLE);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN2, GPIO_PULL_DISABLE);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN3, GPIO_PULL_DISABLE);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN4, GPIO_PULL_DISABLE);


#if  !defined (CONFIG_MTK_C2K_SUPPORT)  //Titan_TL
     if (pin1_val && pin4_val )
     {
         return 1;
     }
     else
     {
         return 0;
     }
#else
     if (0) //( pin2_val && ( !pin3_val) && ( !pin4_val))
     {
         return 1;
     }
     else
     {
         return 0;
     }
#endif
}
#else
int get_pa_num(void)
{
//baixue add for disable second PA
      return 0;

     //add by pare for modem gpio check
#define GPIO_VERSION_PIN1    (GPIO96 | 0x80000000)

	int pin1_val = 0, pin2_val = 0;

      mt_set_gpio_pull_select(GPIO_VERSION_PIN1, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_VERSION_PIN1, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_VERSION_PIN1, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_VERSION_PIN1, GPIO_DIR_IN);

	mdelay(10);

	pin1_val = mt_get_gpio_in(GPIO_VERSION_PIN1);

	printk(KERN_ERR "%s:  pin1 is %d\n",__func__, pin1_val);

      mt_set_gpio_pull_enable(GPIO_VERSION_PIN1, GPIO_PULL_DISABLE);


      return !pin1_val;

}
#endif

static ssize_t RF_PA_info_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
//	u8 string[5]={'\0'};
	int tmp=0;

	tmp = get_pa_num();

	s += sprintf(s, "%d\n",tmp);

	return (s - buf);
}

static struct kobj_attribute RF_PA_info_attr = {
	.attr = {
		.name = "RF_PA_Type",
		.mode = 0444,
	},
	.show =&RF_PA_info_show,
};

//+add by liujingchuan for check cust prj ver
int get_hw_prj(void)
{
#define GPIO_HW_PRJ_PIN0    (GPIO93 | 0x80000000)
#define GPIO_HW_PRJ_PIN1    (GPIO96 | 0x80000000)
	int pin0_val = 0;
	int  pin1_val = 0;
	int hw_prj=0;

      mt_set_gpio_pull_select(GPIO_HW_PRJ_PIN0, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_HW_PRJ_PIN0, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_HW_PRJ_PIN0, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_HW_PRJ_PIN0, GPIO_DIR_IN);

      mt_set_gpio_pull_select(GPIO_HW_PRJ_PIN1, GPIO_PULL_UP);
      mt_set_gpio_pull_enable(GPIO_HW_PRJ_PIN1, GPIO_PULL_ENABLE);
      mt_set_gpio_mode(GPIO_HW_PRJ_PIN1, GPIO_MODE_00);
      mt_set_gpio_dir(GPIO_HW_PRJ_PIN1, GPIO_DIR_IN);

	mdelay(20);

	pin0_val = mt_get_gpio_in(GPIO_HW_PRJ_PIN0);
	pin1_val = mt_get_gpio_in(GPIO_HW_PRJ_PIN1);
	hw_prj = ( pin0_val?1:0);
	hw_prj |= (pin1_val?(1<<1):0);

	printk(KERN_ERR "%s: pin0 is %d , pin1 is %d, hw_prj=%d\n",__func__, pin0_val, pin1_val,hw_prj);

      mt_set_gpio_pull_enable(GPIO_HW_PRJ_PIN0, GPIO_PULL_DISABLE);
      mt_set_gpio_pull_enable(GPIO_HW_PRJ_PIN1, GPIO_PULL_DISABLE);

	return  hw_prj;

}
static ssize_t get_hw_prj_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;

      s += sprintf(s, "%d\n",get_hw_prj());

	return (s - buf);

}

static struct kobj_attribute get_hw_prj_attr = {
	.attr = {
		.name = "hw_prj",
		.mode = 0644,
	},
	.show =&get_hw_prj_show,
};

struct hw_ver {
        const char *name;
        const char *ver;
        int voltage_max;
        int voltage_min;
};

static struct hw_ver hw_ver_array[] = {
        {
                .name = "TL00",
                .ver = "EP",
                .voltage_max = 1800,
                .voltage_min = 1650,
        },
	{
                .name = "AL00",
                .ver = "EP",
                .voltage_max = 150,
                .voltage_min = 0,
        },
};
#if 0
#define BOARD_ID_CHANNEL_NUM 12
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
struct hw_ver * get_hw_ver_info(void)
{
	int data[4], ret_value = 0, hw_ver=0x0;
	int temp_vol;
	int i = 0;
	int hw_ver_num=(sizeof(hw_ver_array))/(sizeof(hw_ver_array[0]));

	ret_value = IMM_GetOneChannelValue(BOARD_ID_CHANNEL_NUM, data, &hw_ver);
        if (ret_value == -1) {/* AUXADC is busy */
		printk(KERN_ERR "IMM_GetOneChannelValua get raw value error \n");
        }
	temp_vol = hw_ver * 1800 / 4096;

	for(i = 0;i<hw_ver_num;i++) {
                if((temp_vol>= hw_ver_array[i].voltage_min)&&(temp_vol< hw_ver_array[i].voltage_max))
                {
                        break;
                }
        }
        if(i>=hw_ver_num)
        {
          	return NULL;
        }

	return &hw_ver_array[i];

}
static ssize_t get_hw_ver_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char *s = buf;
	struct hw_ver * hw_ver_info=get_hw_ver_info();

        if( hw_ver_info==NULL)
        {
          	s += sprintf(s, "%s\n","invalid hw_ver");
        }
	else
	{
		s += sprintf(s, "%s-%s\n",hw_ver_info->ver,hw_ver_info->name);
	}

	return (s - buf);

}

static struct kobj_attribute get_hw_ver_attr = {
	.attr = {
		.name = "hw_ver",
		.mode = 0644,
	},
	.show =&get_hw_ver_show,
};
#endif
//-add by hzb
#endif
#if 0
extern unsigned long Ontim_get_mem_size(void);

static ssize_t get_equip_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
        char *s = buf;
#if 0  //modify by hzb
#define GPIO_HW_VER_PIN0    (GPIO81 | 0x80000000)

        mt_set_gpio_pull_select(GPIO_HW_VER_PIN0, GPIO_PULL_UP);
        mt_set_gpio_pull_enable(GPIO_HW_VER_PIN0, GPIO_PULL_ENABLE);
        mt_set_gpio_mode(GPIO_HW_VER_PIN0, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_HW_VER_PIN0, GPIO_DIR_IN);
        mdelay(20);
	s += sprintf(s, "%s\n",mt_get_gpio_in(GPIO_HW_VER_PIN0) ? "C10" : "C00");
        mt_set_gpio_pull_enable(GPIO_HW_VER_PIN0, GPIO_PULL_DISABLE);
#else

	printk(KERN_ERR "%s: MEM size is %luK \n",__func__,Ontim_get_mem_size());
	s += sprintf(s, "%s\n",(Ontim_get_mem_size() >3145728) ? "C10" : "C00");  //mem size > 3G is C10
#endif
        return (s - buf);

}

static struct kobj_attribute get_equip_attr = {
        .attr = {
                .name = "equip",
                .mode = 0644,
        },
        .show =&get_equip_show,
};

//+add by hzb for check cust prj ver
static void check_cust_ver(void){
    int rb_flag = 0;;
    struct hw_ver * hw_ver_info=get_hw_ver_info();
    if ( hw_ver_info == NULL )
    {
    	printk(KERN_ERR "%s: Get PRJ info Error!!!\n",__func__);
        return;
    }
    printk(KERN_ERR "%s: Build PRJ is %s, This PRJ is %s!!\n",__func__, PRJ_NAME, hw_ver_info->name);
    if((!strcmp(hw_ver_info->name, PRJ_NAME)) || (!strcmp(hw_ver_info->name, "NULL"))){
    	printk(KERN_ERR "%s: Version Pass!!\n",__func__);
    }
    else
    {
        printk(KERN_ERR "%s: Version Error!!!\n",__func__);
       // kernel_restart("prjerr");
    }
}
#endif
//-add by hzb for check cust prj ver

static struct attribute * g[] = {
	&i2c_devices_info_attr.attr,//+add by liuwei
	&back_camera_info_attr.attr,
	&backaux_camera_info_attr.attr,
	&front_camera_info_attr.attr,
	&frontaux_camera_info_attr.attr,
    
 	&back_camera_id_attr.attr,
	&backaux_camera_id_attr.attr,
	&front_camera_id_attr.attr, 
	&frontaux_camera_id_attr.attr,
    
	&gesture_enable_attr.attr,
	&gps_reset_attr.attr,
	&gps_update_attr.attr,
	&gps_avdd_en_attr.attr,
	&gps_prtrg_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};
#if 0
int touchscreen_has_steel_film=0;
static int __init touchscreen_film_setup(char *str)
{
	int en;
	if(!get_option(&str, &en))
		return 0;
	touchscreen_has_steel_film = en;
	return 1;
}

__setup("tp_film=", touchscreen_film_setup);

int get_touchscreen_film_state(void)
{
	printk("[kernel]:touchscreen_has_steel_film=%d.\n",touchscreen_has_steel_film);
	return touchscreen_has_steel_film;
}
#endif
#if 0
int lcm_id=0x83;
static int __init lcm_id_setup(char *str)
{
        int en;
        if(!get_option(&str, &en))
                return 0;
        lcm_id = en;
        return 1;
}
int get_lcm_id(void)
{
        printk("[kernel]:get_lcm_id=%x.\n",lcm_id);
        return lcm_id;
}
__setup("lcm_id=", lcm_id_setup);
#endif

static int __init bootinfo_init(void)
{
	int ret = -ENOMEM;

	//printk("%s,line=%d\n",__func__,__LINE__);
	//eko add gps
	ret = gpio_request(GPS_RESET, "gps_reset");
	if (ret) {
		pr_err("%s:Failed to request GPS RESET GPIO, ret = %d\n", __func__,ret);
		//goto fail;
	}
	gpio_direction_input(GPS_RESET);

	ret = gpio_request(GPS_UPDATE, "gps_update");
	if (ret) {
		pr_err("%s:Failed to request GPS RESET GPIO, ret = %d\n",  __func__,ret);
		//goto fail;
	}
	gpio_direction_input(GPS_UPDATE);

	ret = gpio_request(GPS_AVDD_EN, "gps_avdd_en");
	if (ret) {
		pr_err("%s:Failed to request GPS avdd en GPIO, ret = %d\n", __func__,ret);
		//goto fail;
	}
	gpio_direction_input(GPS_AVDD_EN);

	ret = gpio_request(GPS_PRTRG, "gps_prtrg");
	if (ret) {
		pr_err("%s:Failed to request GPS PRTRG GPIO, ret = %d\n", __func__,ret);
		//goto fail;
	}
	gpio_direction_input(GPS_PRTRG);
    //eko --

	bootinfo_kobj = kobject_create_and_add("ontim_bootinfo", NULL);

	if (bootinfo_kobj == NULL) {
		printk("bootinfo_init: kobject_create_and_add failed\n");
		goto fail;
	}

	ret = sysfs_create_group(bootinfo_kobj, &attr_group);
	if (ret) {
		printk("bootinfo_init: sysfs_create_group failed\n");
		goto sys_fail;
	}

	return ret;
sys_fail:
	kobject_del(bootinfo_kobj);
fail:
	if (gpio_is_valid(GPS_RESET)) {
		gpio_free(GPS_RESET);
		pr_info("remove GPS_RESET success\n");
	}
	if (gpio_is_valid(GPS_UPDATE)) {
		gpio_free(GPS_UPDATE);
		pr_info("remove GPS_UPDATE success\n");
	}
	if (gpio_is_valid(GPS_PRTRG)) {
		gpio_free(GPS_PRTRG);
		pr_info("remove GPS_PRTRG success\n");
	}
	if (gpio_is_valid(GPS_AVDD_EN)) {
		gpio_free(GPS_AVDD_EN);
		pr_info("remove GPS_AVDD_EN success\n");
	}
	return ret;

}

static void __exit bootinfo_exit(void)
{
	if (gpio_is_valid(GPS_RESET)) {
		gpio_free(GPS_RESET);
		pr_info("remove GPS_RESET success\n");
	}
	if (gpio_is_valid(GPS_UPDATE)) {
		gpio_free(GPS_UPDATE);
		pr_info("remove GPS_UPDATE success\n");
	}
	if (gpio_is_valid(GPS_PRTRG)) {
		gpio_free(GPS_PRTRG);
		pr_info("remove GPS_PRTRG success\n");
	}
	if (gpio_is_valid(GPS_AVDD_EN)) {
		gpio_free(GPS_AVDD_EN);
		pr_info("remove GPS_AVDD_EN success\n");
	}
	if (bootinfo_kobj) {
		sysfs_remove_group(bootinfo_kobj, &attr_group);
		kobject_del(bootinfo_kobj);
	}
}

arch_initcall(bootinfo_init);
module_exit(bootinfo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Boot information collector");
