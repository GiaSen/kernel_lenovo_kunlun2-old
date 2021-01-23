#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <soc/qcom/smem.h>
#include <soc/qcom/socinfo.h>
#include <soc/qcom/boot_stats.h>
#include <asm-generic/bug.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/of.h>

#include <linux/fs.h>
#include <asm/system_misc.h>

#include <linux/qpnp/qpnp-adc.h>
#include "board_id_adc.h"

#define BUF_SIZE 64

char front_cam_name[64] = "Unknown";
char frontaux_cam_name[64] = "Unknown";
char back_cam_name[64] = "Unknown";
char backaux_cam_name[64] = "Unknown";
char front_cam_efuse_id[64]={0};
char frontaux_cam_efuse_id[64]={0};
char back_cam_efuse_id[64]={0};
char backaux_cam_efuse_id[64]={0};

#if 0
//read borad ID from adc
#define BOARD_ID_MAX 16
struct qpnp_vadc_result chip_adc_result;

typedef struct adc_voltage {
	int min_voltage;
	int max_voltage;
} adc_boardid_match;

static adc_boardid_match boardid_table[] = {
	{.min_voltage = 150, .max_voltage = 250}, //10k
	{.min_voltage = 250, .max_voltage = 350}, //20k
	{.min_voltage = 350, .max_voltage = 480}, //27k
	{.min_voltage = 480, .max_voltage = 550}, //39k
	{.min_voltage = 550, .max_voltage = 630}, //47k
	{.min_voltage = 630, .max_voltage = 710}, //56k
	{.min_voltage = 710, .max_voltage = 790}, //68k
	{.min_voltage = 790, .max_voltage = 870}, //82k
	{.min_voltage = 870, .max_voltage = 970}, //100k
	{.min_voltage = 970, .max_voltage = 1070}, //120k
	{.min_voltage = 1070, .max_voltage = 1170}, //150k
};
#endif



typedef struct board_id {
	int index;
	const char *hw_version;
	const char *qcn_type;
	const char *model;
} boardid_match_t;

static boardid_match_t board_table[] = {
	{ .index = 0,  .hw_version = "PVT-SPLIT",  .qcn_type = "no-ca-split", .model = "advanced1"  },
	{ .index = 1,  .hw_version = "DVT1",       .qcn_type = "no-ca"      , .model = "primary"    },
	{ .index = 2,  .hw_version = "DVT2",       .qcn_type = "no-ca-del"  , .model = "primary"    },
	{ .index = 3,  .hw_version = "REL",        .qcn_type = "no-ca-del"  , .model = "primary"    },
	{ .index = 4,  .hw_version = "DVT2",       .qcn_type = "no-ca-del"  , .model = "standard"   },
	{ .index = 5,  .hw_version = "DVT2",       .qcn_type = "no-ca"      , .model = "advanced1"  },
	{ .index = 6,  .hw_version = "DVT2-SPLIT", .qcn_type = "no-ca-split", .model = "advanced1"  },
	{ .index = 7,  .hw_version = "REL",        .qcn_type = "no-ca-del"  , .model = "standard"   },
	{ .index = 8,  .hw_version = "EVT",        .qcn_type = "ca"         , .model = "advanced1"  },
	{ .index = 9,  .hw_version = "DVT1",       .qcn_type = "ca"         , .model = "advanced1"  },
	{ .index = 10, .hw_version = "DVT1-SPLIT", .qcn_type = "no-ca-split", .model = "advanced1"  },
	{ .index = 11, .hw_version = "REL-SPLIT",  .qcn_type = "no-ca-split", .model = "advanced2"  },
	{ .index = 12, .hw_version = "DVT2.5",     .qcn_type = "no-ca-del"  , .model = "primary"    },
	{ .index = 13, .hw_version = "PVT",        .qcn_type = "no-ca-del"  , .model = "primary"    },
	{ .index = 14, .hw_version = "PVT",        .qcn_type = "no-ca-del"  , .model = "standard"   },
	{ .index = 15, .hw_version = "REL-SPLIT",  .qcn_type = "no-ca-split", .model = "advanced1"  },
};

typedef struct mid_match {
	int index;
	const char *name;
} mid_match_t;

static mid_match_t emmc_table[] = {
	{
		.index = 0,
		.name = "Unknown"
	},
	{
		.index = 17,
		.name = "Toshiba"
	},
	{
		.index = 19,
		.name = "Micron"
	},
	{
		.index = 69,
		.name = "Sandisk"
	},
	{
		.index = 21,
		.name = "Samsung"
	},
	{
		.index = 0x90,
		.name = "Hynix"
	},
	{
		.index = 0x70,
		.name = "KSI"
	},
	/*UFS*/
	{
		.index = 0xCE,
		.name = "Samsung"
	},
	{
		.index = 0xAD,
		.name = "Hynix"
	},
	{
		.index = 0x98,
		.name = "Toshiba"
	},
};

static mid_match_t lpddr_table[] = {
	{
		.index = 0,
		.name = "UNKNOWN"
	},
	{
		.index = 0xff,
		.name = "Micron"
	},
	{
		.index = 6,
		.name = "Hynix"
	},
	{
		.index = 1,
		.name = "Samsung"
	},
	{
		.index = 3,
		.name = "Micron"
	},
};

#define MAX_HWINFO_SIZE 64
#include "hwinfo.h"
typedef struct {
	char *hwinfo_name;
	char hwinfo_buf[MAX_HWINFO_SIZE];
} hwinfo_t;

#define KEYWORD(_name) \
	[_name] = {.hwinfo_name = __stringify(_name), \
		   .hwinfo_buf = {0}},

static hwinfo_t hwinfo[HWINFO_MAX] =
{
#include "hwinfo.h"
};
#undef KEYWORD

static const char *foreach_lpddr_table(int index)
{
	int i = 0;

	for (; i < sizeof(lpddr_table) / sizeof(mid_match_t); i++) {
		if (index == lpddr_table[i].index)
			return lpddr_table[i].name;
	}

	return lpddr_table[0].name;
}

static const char *foreach_emmc_table(int index)
{
	int i = 0;

	for (; i < sizeof(emmc_table) / sizeof(mid_match_t); i++) {
		if (index == emmc_table[i].index)
			return emmc_table[i].name;
	}

	return emmc_table[0].name;;
}
static int hwinfo_read_file(char *file_name, char buf[], int buf_size)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos = 0;
	ssize_t len = 0;

	if (file_name == NULL || buf == NULL)
		return -1;

	fp = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		printk(KERN_CRIT "file not found/n");
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	len = vfs_read(fp, buf, buf_size, &pos);
	buf[buf_size - 1] = '\n';
	printk(KERN_INFO "buf= %s,size = %ld \n", buf, len);
	filp_close(fp, NULL);
	set_fs(fs);

	return 0;
}
#if 0
/*Android:Settings->About phone->CPU  register function to distinguish the CPU model*/
static char *msm_read_hardware_id(void)
{
	static char msm_soc_str[256] = "Qualcomm Technologies, Inc ";
	static bool string_generated;
	int ret = 0;

	if (string_generated)
		return msm_soc_str;

	ret = get_cpu_type();
	if (ret != 0)
		goto err_path;

	ret = strlcat(msm_soc_str, hwinfo[CPU_TYPE].hwinfo_buf,
	              sizeof(msm_soc_str));
	if (ret > sizeof(msm_soc_str))
		goto err_path;

	string_generated = true;
	return msm_soc_str;
err_path:
	printk(KERN_CRIT "UNKNOWN SOC TYPE, Using defaults.\n");
	return "Qualcomm Technologies, Inc SDM710";
}
#endif
static int  get_cpu_type(void)
{
	sprintf(hwinfo[CPU_TYPE].hwinfo_buf, "%s",arch_read_hardware_id());
	return 0;
}
#if 0
char lcd_name[64] = "unknow";
static int get_lcd_type(void)
{
	sprintf(hwinfo[LCD_MFR].hwinfo_buf, "%s", lcd_name);
	return 1;
}
//__setup("msm_drm.dsi_display0=", set_lcd_name);
#else
//#define LCD_INFO_FILE "/sys/class/graphics/fb0/msm_fb_panel_info"
#define LCD_INFO_FILE "/sys/ontim_dev_debug/touch_screen/vendor"
static int get_lcd_type(void)
{
	char buf[200] = {0};
	int  ret = 0;

	ret = hwinfo_read_file(LCD_INFO_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get lcd_type read file failed.\n");
		return -1;
	}
	printk(KERN_INFO "lcd %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	memcpy(hwinfo[LCD_MFR].hwinfo_buf, buf, strlen(buf));

	return 0;
}
#endif
#define TP_VENDOR_FILE "/sys/ontim_dev_debug/touch_screen/vendor"
#define TP_VERSION_FILE "/sys/ontim_dev_debug/touch_screen/version"
static int get_tp_info(void)
{
	char buf[64] = {0};
	char buf2[64] = {0};
	char str[128] = {0};
	int  ret = 0;

	ret = hwinfo_read_file(TP_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_tp_info failed.");
		return -1;
	}
	printk(KERN_INFO "tp %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	ret = hwinfo_read_file(TP_VERSION_FILE, buf2, sizeof(buf2));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_tp_info failed.");
		return -1;
	}
	printk(KERN_INFO "tp version %s\n", buf2);
	if (buf2[strlen(buf2) - 1] == '\n')
		buf2[strlen(buf2) - 1] = '\0';

	sprintf(str, "%s %s", buf, buf2);
	memcpy(hwinfo[TP_MFR].hwinfo_buf, str , strlen(str));
	return 0;
}

#define BATTARY_RESISTANCE_FILE "/sys/class/power_supply/battery/resistance_id"
static int get_battary_mfr(void)
{
	char buf[20] = {0};
	int ret = 0;
	int resistance_value = 0;

	ret = hwinfo_read_file(BATTARY_RESISTANCE_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_mfr failed.");
		return -1;
	}
	printk(KERN_INFO "Battary %s\n", buf);
	sscanf(buf, "%d", &resistance_value);

	strcpy(hwinfo[BATTARY_MFR].hwinfo_buf, (resistance_value >= 85000) ? "SCUD" : "ATL");

	return 0;
}

#define BATTARY_CAPACITY_FILE "/sys/class/power_supply/battery/capacity"
static int get_battary_cap(void)
{
	char buf[20] = {0};
	int ret = 0;
	int capacity_value = 0;

	ret = hwinfo_read_file(BATTARY_CAPACITY_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_cap failed.");
		return -1;
	}
	printk(KERN_INFO "Battary cap %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	sscanf(buf, "%d", &capacity_value);

	strcpy(hwinfo[BATTARY_CAP].hwinfo_buf, buf);

	return 0;
}

#define BATTARY_VOL_FILE "/sys/class/power_supply/battery/voltage_now"
static int get_battary_vol(void)
{
	char buf[20] = {0};
	int ret = 0;
	int voltage_now_value = 0;

	ret = hwinfo_read_file(BATTARY_VOL_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_battary_vol failed.");
		return -1;
	}
	printk(KERN_INFO "Battary vol %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';
	sscanf(buf, "%d", &voltage_now_value);

	strcpy(hwinfo[BATTARY_VOL].hwinfo_buf, buf);

	return 0;
}

#define TYPEC_VENDOR_FILE "/sys/class/power_supply/usb/typec_mode"
static ssize_t get_typec_vendor(void)
{
	char buf[64] = {0};
	int ret = 0;

	ret = hwinfo_read_file(TYPEC_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get_typec_vendor failed.");
		return -1;
	}
	buf[63] = '\n';
	printk(KERN_INFO "Typec vendor: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	memcpy(hwinfo[TYPEC_MFR].hwinfo_buf, buf, 64);

	return 0;
}

#define TYPEC_CC_STATUS_FILE "/sys/class/usbpd/usbpd0/get_cc_status"
static ssize_t get_typec_cc_status(void)
{
	char buf[BUF_SIZE] = {0};
	int ret = 0;

	ret = hwinfo_read_file(TYPEC_CC_STATUS_FILE, buf, BUF_SIZE);
	if (ret != 0) {
		printk(KERN_CRIT "get_typec_cc status failed.");
		return -1;
	}
	buf[1] = '\0';
	printk(KERN_INFO "Typec cc status: %s\n", buf);

	memcpy(hwinfo[TYPEC_CC_STATUS].hwinfo_buf, buf, BUF_SIZE);

	return 0;
}

#define ADB_SN_FILE "/config/usb_gadget/g2/strings/0x409/serialnumber"
static ssize_t get_adb_sn(void)
{
	char buf[16] = {};
	int ret = 0;

	ret = hwinfo_read_file(ADB_SN_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get adb sn failed.");
		return -1;
	}
	printk(KERN_INFO "Adb SN: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	strcpy(hwinfo[ADB_SN].hwinfo_buf, buf);

	return 0;
}

#define SPEAKER_MFR_FILE "/proc/asound/cards"
static int get_speaker_mfr(void)
{
	char buf[MAX_HWINFO_SIZE] = {0};
	char* p = buf;
	char* q = buf;
	int len = 0;
	int ret = -1;

	ret = hwinfo_read_file(SPEAKER_MFR_FILE, buf, sizeof(buf));
	if (ret != 0)
	{
		printk(KERN_CRIT "get_speaker_mfr failed.");
		return -1;
	}

	while ((*p++ != '[') && (p < (buf + MAX_HWINFO_SIZE)));
	while ((*p != ']') && (p < (buf + MAX_HWINFO_SIZE)))
	{
		*q++ = *p++;
		len++;
	}

	buf[len - 1] = '\0';
	printk(KERN_INFO "speaker %s\n", buf);

	memcpy(hwinfo[SPEAKER_MFR].hwinfo_buf, buf, len);
	return 0;
}

//houzn add
#define CHARGER_IC_VENDOR_FILE "/sys/ontim_dev_debug/charge_ic/vendor"
static void get_charger_ic(void)
{
	int ret = 0;
	char buf[MAX_HWINFO_SIZE] = {0};

	sprintf(buf, "%s", "Unknow");
	ret = hwinfo_read_file(CHARGER_IC_VENDOR_FILE, buf, sizeof(buf));
	if (ret)
		pr_err("get %s finger type failed.\n", CHARGER_IC_VENDOR_FILE);

	if (likely(buf[strlen(buf) - 1] == '\n')) {
		buf[strlen(buf) - 1] = '\0';
	}
	strcpy(hwinfo[CHARGER_IC_MFR].hwinfo_buf, buf);
	pr_err("charge_ic:%s .\n", hwinfo[CHARGER_IC_MFR].hwinfo_buf);
}
//add end

#define FINGERPRINT_VENDOR_FILE "/sys/ontim_dev_debug/fingersensor/vendor"
static ssize_t get_fingerprint_id(void)
{
	char buf[MAX_HWINFO_SIZE] = {};
	int ret = 0;

	ret = hwinfo_read_file(FINGERPRINT_VENDOR_FILE, buf, sizeof(buf));
	if (ret != 0) {
		printk(KERN_CRIT "get_fingerprint_vendor failed.");
		return -1;
	}
	printk(KERN_INFO "Fingerprint vendor: %s\n", buf);
	if (buf[strlen(buf) - 1] == '\n')
		buf[strlen(buf) - 1] = '\0';

	strcpy(hwinfo[FP_MFR].hwinfo_buf, buf);

	return 0;
}

static void get_front_camera_id(void)
{
	if (front_cam_name != NULL)
		strncpy(hwinfo[FRONT_CAM_MFR].hwinfo_buf, front_cam_name,
		        ((strlen(front_cam_name) >= sizeof(hwinfo[FRONT_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[FRONT_CAM_MFR].hwinfo_buf) : strlen(front_cam_name))));
}
static void get_frontaux_camera_id(void)
{
	if (frontaux_cam_name != NULL)
		strncpy(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf, frontaux_cam_name,
		        ((strlen(frontaux_cam_name) >= sizeof(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[FRONTAUX_CAM_MFR].hwinfo_buf) : strlen(frontaux_cam_name))));
}
static void get_back_camera_id(void)
{
	if (back_cam_name != NULL)
		strncpy(hwinfo[BACK_CAM_MFR].hwinfo_buf, back_cam_name,
		        ((strlen(back_cam_name) >= sizeof(hwinfo[BACK_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[BACK_CAM_MFR].hwinfo_buf) : strlen(back_cam_name))));
}
static void get_backaux_camera_id(void)
{
	if (backaux_cam_name != NULL)
		strncpy(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf, backaux_cam_name,
		        ((strlen(backaux_cam_name) >= sizeof(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX_CAM_MFR].hwinfo_buf) : strlen(backaux_cam_name))));
}

static void get_front_camera_efuse_id(void)
{
	if (front_cam_efuse_id != NULL)
		strncpy(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf, front_cam_efuse_id,
		        ((strlen(front_cam_efuse_id) >= sizeof(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[FRONT_CAM_EFUSE].hwinfo_buf) : strlen(front_cam_efuse_id))));
}
static void get_frontaux_camera_efuse_id(void)
{
	if (frontaux_cam_efuse_id != NULL)
		strncpy(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf, frontaux_cam_efuse_id,
		        ((strlen(frontaux_cam_efuse_id) >= sizeof(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[FRONTAUX_CAM_EFUSE].hwinfo_buf) : strlen(frontaux_cam_efuse_id))));
}
static void get_back_camera_efuse_id(void)
{
	if (back_cam_efuse_id != NULL)
		strncpy(hwinfo[BACK_CAM_EFUSE].hwinfo_buf, back_cam_efuse_id,
		        ((strlen(back_cam_efuse_id) >= sizeof(hwinfo[BACK_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[BACK_CAM_EFUSE].hwinfo_buf) : strlen(back_cam_efuse_id))));
}
static void get_backaux_camera_efuse_id(void)
{
	if (backaux_cam_efuse_id != NULL)
		strncpy(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf, backaux_cam_efuse_id,
		        ((strlen(backaux_cam_efuse_id) >= sizeof(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf) ?
		          sizeof(hwinfo[BACKAUX_CAM_EFUSE].hwinfo_buf) : strlen(backaux_cam_efuse_id))));
}

static void get_card_present(void)
{
    char card_holder_present[BUF_SIZE];

    memset(card_holder_present, '\0', BUF_SIZE);
    strncpy(card_holder_present, "truly", 5);
    if (gpio_get_value(96) == 1)
    {
        memset(card_holder_present, '\0', BUF_SIZE);
        strncpy(card_holder_present, "false", 5);
    }
    memset(hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf, 0x00,sizeof(hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf));
    strncpy(hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf, card_holder_present,
            ((strlen(card_holder_present) >= sizeof(hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf) ?
              sizeof(hwinfo[CARD_HOLDER_PRESENT].hwinfo_buf) : strlen(card_holder_present))));
}

static int get_pon_reason(void)
{
	char pon_reason_info[32] = "UNKNOWN";

	/*	switch ((get_boot_reason() & 0xFF))
		{
		case 0x20:
			pon_reason_info = "usb charger";
			break;
		case 0x21:
			pon_reason_info = "soft reboot";
			break;
		case 0xa0:
			pon_reason_info = "power key";
			break;
		case 0xa1:
			pon_reason_info = "hard reset";
			break;
		default:
			pon_reason_info = "unknow";
			break;
		}*/

	return sprintf(hwinfo[pon_reason].hwinfo_buf, "%s", pon_reason_info);
}

static int get_secure_boot_version(void)
{
	char is_secureboot[32] = "Unknown";
	/*	if (get_secure_boot_value())
			is_secureboot = "SE";
		else
			is_secureboot = "NSE";*/

	return sprintf(hwinfo[secboot_version].hwinfo_buf, "%s", is_secureboot);
}

unsigned int platform_board_id = 0;
EXPORT_SYMBOL(platform_board_id);
static int get_version_id(void)
{
#if 1
	int id = platform_board_id;
	return sprintf(hwinfo[board_id].hwinfo_buf, "%04d", id);
#else
	int rc = 0;
	int voltage = 0;
	int board_id_num = 0;
	int id = 0;

	if (NULL == chip_adc) {
		pr_err("ontim: %s: chip_adc is NULL\n", __func__);
		return -ENOMEM;
	}

	rc = qpnp_vadc_read(chip_adc->vadc_dev, chip_adc->vadc_mux, &chip_adc_result);
	if (rc) {
		pr_err("ontim: %s: qpnp_vadc_read failed(%d)\n",
		       __func__, rc);
	} else {
		voltage = (int)chip_adc_result.physical / 1000;
	}
	for (board_id_num = 0; board_id_num < sizeof(boardid_table) / sizeof(adc_boardid_match); board_id_num++) {
		if ( voltage > boardid_table[board_id_num].min_voltage &&
		        voltage < boardid_table[board_id_num].max_voltage ) {
			id = board_id_num;
		}
	}
	printk("hwinfo id=%04d,board_id_num=%d,chip_adc->voltage=%d\n", id, board_id_num, voltage);
	return sprintf(hwinfo[board_id].hwinfo_buf, "%d", id);
#endif	
}

static int get_qcn_type(void)
{
	int id = platform_board_id;
	if (id > (sizeof(board_table) / sizeof(boardid_match_t) - 1))
		id = sizeof(board_table) / sizeof(boardid_match_t) - 1;
	return sprintf(hwinfo[qcn_type].hwinfo_buf, "%s", board_table[id].qcn_type);
}

char NFC_BUF[MAX_HWINFO_SIZE] = {"Unknow"};
EXPORT_SYMBOL(NFC_BUF);
static void get_nfc_deviceinfo(void)
{
	strcpy(hwinfo[NFC_MFR].hwinfo_buf, NFC_BUF);
}

static int set_serialno(char *src)
{
	if (src == NULL)
		return 0;
	sprintf(hwinfo[serialno].hwinfo_buf, "%s", src);
	return 1;
}
__setup("androidboot.serialno=", set_serialno);

//get emmc info
char pMeminfo[MAX_HWINFO_SIZE] = {'\0'};
static int set_memory_info(char *src)
{
	if (src == NULL)
		return 0;
	sprintf(pMeminfo, "%s", src);
	return 1;
}
__setup("memory_info=", set_memory_info);

int _atoi(char * str)
{
	int value = 0;
	int sign = 1;
	int radix;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
	{
		radix = 16;
		str += 2;
	}
	else if (*str == '0')
	{
		radix = 8;
		str++;
	} else {
		radix = 10;
	}
	while (*str && *str != '\0')
	{
		if (radix == 16)
		{
			if (*str >= '0' && *str <= '9')
				value = value * radix + *str - '0';
			else if (*str >= 'A' && *str <= 'F')
				value = value * radix + *str - 'A' + 10;
			else if (*str >= 'a' && *str <= 'f')
				value = value * radix + *str - 'a' + 10;
		} else {
			value = value * radix + *str - '0';
		}
		str++;
	}
	return sign * value;
}

#define BYTE(_x) (_x<<0x03)
#define EMMC_MANFID_FILE "/sys/class/mmc_host/mmc0/mmc0:0001/manfid"
#define EMMC_SIZE_FILE   "/sys/class/mmc_host/mmc0/mmc0:0001/block/mmcblk0/size"
#define LPDDR_SIZE_FILE   "/proc/meminfo"
static void get_emmc_info(void)
{
	unsigned int  hwinfo_value = 0;
	unsigned char emmc_mid = 0, lpddr_mid = 0;
	int lpddr_cap = 0, emmc_cap = 0;
	const char *emmc_mid_name, *lpddr_mid_name;
	char buf[MAX_HWINFO_SIZE] = {'\0'};
	char* p = buf;
	char* q = buf;
	int ret = 0;

	if (pMeminfo[0] > '1')
	{
		hwinfo_value = _atoi(pMeminfo);
		lpddr_mid = hwinfo_value >> BYTE(0) & 0xFF;
		emmc_cap = hwinfo_value >> BYTE(1) & 0xFF;
		emmc_mid = hwinfo_value >> BYTE(2) & 0xFF;
	} else {
		ret = hwinfo_read_file(EMMC_MANFID_FILE, buf, sizeof(buf));
		if (ret != 0) {
			printk(KERN_CRIT "EMMC_MANFID_FILE failed.");
			return;
		}
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		emmc_mid = _atoi(buf);

		memset(buf, 0x00, sizeof(buf));
		ret = hwinfo_read_file(EMMC_SIZE_FILE, buf, sizeof(buf));
		if (ret != 0) {
			printk(KERN_CRIT "EMMC_SIZE_FILE failed.");
			return;
		}
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		emmc_cap = _atoi(buf) / (1024 * 1024 * 2);

		memset(buf, 0x00, sizeof(buf));
		ret = hwinfo_read_file(LPDDR_SIZE_FILE, buf, sizeof(buf));
		if (ret != 0) {
			printk(KERN_CRIT "LPDDR_SIZE_FILE failed.");
			return;
		}

		while (*p++ != ':' && p < buf + MAX_HWINFO_SIZE);
		while (*p++ == ' ' && p < buf + MAX_HWINFO_SIZE);
		p = p-1;
		q = p;
		while (*q++ != ' ' && q < buf + MAX_HWINFO_SIZE);
		memcpy(buf, p, q-p-1);
		buf[q-p-1] = '\0';
		buf[q-p]   = '\0';
		lpddr_cap = _atoi(buf)/(1024*1024);
		printk("%s: buf:%s i = %d\n", __func__,buf, lpddr_cap);
	}
	printk("%s:hwinfo_value=0x%08x, lpddr_mid=%#x lpddr_cap=%#x emmc_cap=%#x emmc_mid=%#x\n", \
	       __func__, hwinfo_value, lpddr_mid, lpddr_cap, emmc_cap, emmc_mid);

	emmc_mid_name = foreach_emmc_table(emmc_mid);
	WARN((emmc_mid_name == NULL), "cannot recognize emmc mid=0x%x", emmc_mid);
	if (emmc_mid_name == NULL)
		emmc_mid_name = "Unknown";
	strncpy(hwinfo[emmc_mfr].hwinfo_buf, emmc_mid_name, strlen(emmc_mid_name));
	sprintf(hwinfo[emmc_capacity].hwinfo_buf, "%dGb", emmc_cap);

	lpddr_mid_name = foreach_lpddr_table(lpddr_mid);
	WARN((lpddr_mid_name == NULL), "cannot recognize lpddr mid=0x%x", lpddr_mid);
	if (lpddr_mid_name == NULL)
		lpddr_mid_name = "Unknown";
	//strncpy(hwinfo[lpddr_mfr].hwinfo_buf, lpddr_mid_name, strlen(lpddr_mid_name));
	strncpy(hwinfo[lpddr_mfr].hwinfo_buf, emmc_mid_name, strlen(emmc_mid_name));
	sprintf(hwinfo[lpddr_capacity].hwinfo_buf, "%dGb", lpddr_cap);

}

static ssize_t hwinfo_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	int i = 0;
	printk(KERN_INFO "hwinfo sys node %s \n", attr->attr.name);

	for (; i < HWINFO_MAX && strcmp(hwinfo[i].hwinfo_name, attr->attr.name) && ++i;);

	switch (i)
	{
	case CPU_TYPE:
		get_cpu_type();
		break;
	case NFC_MFR:
		get_nfc_deviceinfo();
		break;
	case SPEAKER_MFR:
		get_speaker_mfr();
		break;
	case BATTARY_MFR:
		get_battary_mfr();
		break;
	case BATTARY_VOL:
		get_battary_vol();
		break;
	case BATTARY_CAP:
		get_battary_cap();
		break;
	case board_id:
		get_version_id();
		break;
	case qcn_type:
		get_qcn_type();
		break;
#if 1
	case LCD_MFR:
		get_lcd_type();
		break;
#endif
	case TP_MFR:
		get_tp_info();
		break;
	case TYPEC_MFR:
		get_typec_vendor();
		break;
    case TYPEC_CC_STATUS:
		get_typec_cc_status();
		break;
	case ADB_SN:
		get_adb_sn();
		break;
	case FRONT_CAM_MFR:
		get_front_camera_id();
		break;
	case FRONTAUX_CAM_MFR:
		get_frontaux_camera_id();
		break;
	case BACK_CAM_EFUSE:
		get_back_camera_efuse_id();
		break;
	case BACKAUX_CAM_EFUSE:
		get_backaux_camera_efuse_id();
		break;
	case FRONT_CAM_EFUSE:
		get_front_camera_efuse_id();
		break;
	case FRONTAUX_CAM_EFUSE:
		get_frontaux_camera_efuse_id();
		break;
	case BACK_CAM_MFR:
		get_back_camera_id();
		break;
	case BACKAUX_CAM_MFR:
		get_backaux_camera_id();
		break;
	case FP_MFR:
		get_fingerprint_id();
		break;
	case CARD_HOLDER_PRESENT:
		get_card_present();
		break;
	case CHARGER_IC_MFR: //houzn add
		get_charger_ic();
		break;
	case pon_reason:
		get_pon_reason();
		break;
	case secboot_version:
		get_secure_boot_version();
		break;
	case emmc_mfr:
	case emmc_capacity:
	case lpddr_mfr:
	case lpddr_capacity:
		get_emmc_info();
		break;
	default:
		break;
	}
	return sprintf(buf, "%s=%s \n",  attr->attr.name, ((i >= HWINFO_MAX || hwinfo[i].hwinfo_buf[0] == '\0') ? "unknow" : hwinfo[i].hwinfo_buf));
}

static ssize_t hwinfo_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{
	return n;
}
#define KEYWORD(_name) \
    static struct kobj_attribute hwinfo##_name##_attr = {   \
                .attr   = {                             \
                        .name = __stringify(_name),     \
                        .mode = 0444,                   \
                },                                      \
            .show   = hwinfo_show,                 \
            .store  = hwinfo_store,                \
        };

#include "hwinfo.h"
#undef KEYWORD

#define KEYWORD(_name)\
    [_name] = &hwinfo##_name##_attr.attr,

static struct attribute * g[] = {
#include "hwinfo.h"
	NULL
};
#undef KEYWORD

static struct attribute_group attr_group = {
	.attrs = g,
};

int ontim_hwinfo_register(enum HWINFO_E e_hwinfo, char *hwinfo_name)
{
	if ((e_hwinfo >= HWINFO_MAX) || (hwinfo_name == NULL))
		return -1;
	strncpy(hwinfo[e_hwinfo].hwinfo_buf, hwinfo_name, \
	        (strlen(hwinfo_name) >= 20 ? 19 : strlen(hwinfo_name)));
	return 0;
}
EXPORT_SYMBOL(ontim_hwinfo_register);

static int __init hwinfo_init(void)
{
	struct kobject *k_hwinfo = NULL;

	if ( (k_hwinfo = kobject_create_and_add("hwinfo", NULL)) == NULL ) {
		printk(KERN_ERR "%s:hwinfo sys node create error \n", __func__);
	}

	if ( sysfs_create_group(k_hwinfo, &attr_group) ) {
		printk(KERN_ERR "%s: sysfs_create_group failed\n", __func__);
	}
	//arch_read_hardware_id = msm_read_hardware_id;
	return 0;
}

static void __exit hwinfo_exit(void)
{
	return ;
}

late_initcall_sync(hwinfo_init);
module_exit(hwinfo_exit);
MODULE_AUTHOR("eko@ontim.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Product Hardward Info Exposure");
