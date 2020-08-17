/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "cam_eeprom_dev.h"
#include "cam_req_mgr_dev.h"
#include "cam_eeprom_soc.h"
#include "cam_eeprom_core.h"
#include "cam_debug_util.h"
/* Huaqin add for biaoding by chenjun at 2018/10/08 start*/
#ifdef huaqin_calibration//chenjun biaoding
#include "cam_sensor_cmn_header.h"
#include <linux/fs.h>
#define RECALIBRATION_FILE_NAME_MASTER "/persist/recalibration_16b10_8856.bin"
#define RECALIBRATION_FILE_NAME_SLAVE "/persist/recalibration_16b10_s5k5e9.bin"
#define  EEPROM_ONE_BURST_SIZE   8
#define  FILE_DATA_SIZE  2048
#define  START_REGADDR_M 0xC6	//should modify by module
#define  SLAVE_ADDR_M 0xA0 >> 1	//should modify by module
#define  START_REGADDR_S 0x13A5	//should modify by module
#define  SLAVE_ADDR_S 0xA0 >> 1	//should modify by module
//uint8_t *buffer1;
//uint8_t *buffer;
#ifdef CHECK_SUM_CRC
char *data_count;
#endif
#endif
/* Huaqin add for biaoding by chenjun at 2018/10/08 end*/
static long cam_eeprom_subdev_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, void *arg)
{
	int                       rc     = 0;
	struct cam_eeprom_ctrl_t *e_ctrl = v4l2_get_subdevdata(sd);

	switch (cmd) {
	case VIDIOC_CAM_CONTROL:
		rc = cam_eeprom_driver_cmd(e_ctrl, arg);
		break;
	default:
		rc = -ENOIOCTLCMD;
		break;
	}

	return rc;
}

static int cam_eeprom_subdev_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	struct cam_eeprom_ctrl_t *e_ctrl =
		v4l2_get_subdevdata(sd);

	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "e_ctrl ptr is NULL");
			return -EINVAL;
	}

	mutex_lock(&(e_ctrl->eeprom_mutex));
	cam_eeprom_shutdown(e_ctrl);
	mutex_unlock(&(e_ctrl->eeprom_mutex));

	return 0;
}

int32_t cam_eeprom_update_i2c_info(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_eeprom_i2c_info_t *i2c_info)
{
	struct cam_sensor_cci_client        *cci_client = NULL;

	if (e_ctrl->io_master_info.master_type == CCI_MASTER) {
		cci_client = e_ctrl->io_master_info.cci_client;
		if (!cci_client) {
			CAM_ERR(CAM_EEPROM, "failed: cci_client %pK",
				cci_client);
			return -EINVAL;
		}
		cci_client->cci_i2c_master = e_ctrl->cci_i2c_master;
		cci_client->sid = (i2c_info->slave_addr) >> 1;
		cci_client->retries = 3;
		cci_client->id_map = 0;
		cci_client->i2c_freq_mode = i2c_info->i2c_freq_mode;
	} else if (e_ctrl->io_master_info.master_type == I2C_MASTER) {
		e_ctrl->io_master_info.client->addr = i2c_info->slave_addr;
		CAM_DBG(CAM_EEPROM, "Slave addr: 0x%x", i2c_info->slave_addr);
	}
	return 0;
}

#ifdef CONFIG_COMPAT
static long cam_eeprom_init_subdev_do_ioctl(struct v4l2_subdev *sd,
	unsigned int cmd, unsigned long arg)
{
	struct cam_control cmd_data;
	int32_t rc = 0;

	if (copy_from_user(&cmd_data, (void __user *)arg,
		sizeof(cmd_data))) {
		CAM_ERR(CAM_EEPROM,
			"Failed to copy from user_ptr=%pK size=%zu",
			(void __user *)arg, sizeof(cmd_data));
		return -EFAULT;
	}

	switch (cmd) {
	case VIDIOC_CAM_CONTROL:
		rc = cam_eeprom_subdev_ioctl(sd, cmd, &cmd_data);
		if (rc < 0) {
			CAM_ERR(CAM_EEPROM,
				"Failed in eeprom suddev handling rc %d",
				rc);
			return rc;
		}
		break;
	default:
		CAM_ERR(CAM_EEPROM, "Invalid compat ioctl: %d", cmd);
		rc = -EINVAL;
	}

	if (!rc) {
		if (copy_to_user((void __user *)arg, &cmd_data,
			sizeof(cmd_data))) {
			CAM_ERR(CAM_EEPROM,
				"Failed to copy from user_ptr=%pK size=%zu",
				(void __user *)arg, sizeof(cmd_data));
			rc = -EFAULT;
		}
	}
	return rc;
}
#endif

static const struct v4l2_subdev_internal_ops cam_eeprom_internal_ops = {
	.close = cam_eeprom_subdev_close,
};

static struct v4l2_subdev_core_ops cam_eeprom_subdev_core_ops = {
	.ioctl = cam_eeprom_subdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = cam_eeprom_init_subdev_do_ioctl,
#endif
};

static struct v4l2_subdev_ops cam_eeprom_subdev_ops = {
	.core = &cam_eeprom_subdev_core_ops,
};

static int cam_eeprom_init_subdev(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;

	e_ctrl->v4l2_dev_str.internal_ops = &cam_eeprom_internal_ops;
	e_ctrl->v4l2_dev_str.ops = &cam_eeprom_subdev_ops;
	strlcpy(e_ctrl->device_name, CAM_EEPROM_NAME,
		sizeof(e_ctrl->device_name));
	e_ctrl->v4l2_dev_str.name = e_ctrl->device_name;
	e_ctrl->v4l2_dev_str.sd_flags =
		(V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS);
	e_ctrl->v4l2_dev_str.ent_function = CAM_EEPROM_DEVICE_TYPE;
	e_ctrl->v4l2_dev_str.token = e_ctrl;

	rc = cam_register_subdev(&(e_ctrl->v4l2_dev_str));
	if (rc)
		CAM_ERR(CAM_SENSOR, "Fail with cam_register_subdev");

	return rc;
}
/* Huaqin add for biaoding by chenjun at 2018/10/08 start*/
#ifdef huaqin_calibration
int recalibration_file_rw(const char *fname, void *buf, loff_t offset, unsigned long len) 
{
	struct file *filep;
	int ret;

	filep = filp_open(fname, O_RDWR | O_CREAT, 0666);
	if(IS_ERR(filep)) {
		printk("\r\n chenjun=>RST: %s error\n", __func__);
		return -1;
	}
	ret = kernel_read(filep, offset, buf, len);
	if (ret < 0) {
		return ret;
	}

	filp_close(filep, NULL);

	return 0;
}

#ifdef CHECK_SUM_CRC
/*check_sum_CRC*/
static char *MakeCRC(void)
{
	static char Res[17];                                 // CRC Result
	char CRC[16];
	int  i;
	char DoInvert;
	for (i=0; i<16; ++i)  CRC[i] = 0;                    // Init before calculation

	for (i=0; i<((int)strlen(data_count)); ++i)
	{
		DoInvert = ('1'==data_count[i]) ^ CRC[15];         // XOR required?

		CRC[15] = CRC[14] ^ DoInvert;
		CRC[14] = CRC[13];
		CRC[13] = CRC[12];
		CRC[12] = CRC[11];
		CRC[11] = CRC[10];
		CRC[10] = CRC[9];
		CRC[9] = CRC[8];
		CRC[8] = CRC[7];
		CRC[7] = CRC[6];
		CRC[6] = CRC[5];
		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2];
		CRC[2] = CRC[1] ^ DoInvert;
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}

	for (i=0; i<16; ++i)  Res[15-i] = CRC[i] ? '1' : '0'; // Convert binary to ASCII
	Res[16] = 0;                                         // Set string terminator

	return(Res);
}
static int GetCRCChecksum(int dataCount,uint8_t  *buffer)
{
	int checksum=0;
	//char *temp;
	char *Result;
	//char data_count[dataCount*8+1];
	int k = 0;
	int i,j;
	int res;
	pr_err("GetCRCChecksum 0");
	//msleep(1000);
	data_count=vmalloc(2048*8+1);
	//temp = data_count;
	pr_err("GetCRCChecksum 1");
	//msleep(3000);
	for(j = 0;j < dataCount ;j++)
	{
		char tt[9] = {0};
		for ( i = 0; i < 8; i++)
		{
			tt[i]  = ((((*(buffer +j)) >>(7-i))&1)==1)?'1':'0';
			*(data_count+k) = tt[i];
			k++;
		}
	}
	*(data_count+dataCount*8) = 0;
	pr_err("GetCRCChecksum 2");
	//msleep(3000);
	Result = MakeCRC();
	pr_err("GetCRCChecksum 3");
	//msleep(3000);
	res=0;
	for (i=0;i<16;i++)
		res+=(Result[i]-48)<<(15-i);
	checksum=res;
	return checksum;
}
#endif

static ssize_t read_eeprom_show(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
	int error;
	int read_eeprom_data;
	//int read_eeprom_data1;
	uint32_t i;
	
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = 1;
	
	error = camera_io_init(&(eeprom->io_master_info));
	if (error) {
		CAM_ERR(CAM_EEPROM, "\r\n chenjun=>read_eeprom_store cci_init failed");
		return -EINVAL;
	}
	/*
	for(i=0;i<100;i++)
	{
	error = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
		0xc6+i, &read_eeprom_data,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_BYTE);
	CAM_ERR(CAM_EEPROM, "\r\n chenjun=>read_eeprom_store read_eeprom_data=0x%x",read_eeprom_data);
	}*/
	for(i=0;i<2;i++)
	{
	error = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
		0x8c6+i, &read_eeprom_data,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_BYTE);
	CAM_ERR(CAM_EEPROM, "\r\n chenjun=>check_sum_crc_master=0x%x",read_eeprom_data);
	error = cam_cci_i2c_read(eeprom->io_master_info.cci_client,
		0x1BA5+i, &read_eeprom_data,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_BYTE);
	CAM_ERR(CAM_EEPROM, "\r\n chenjun=>check_sum_crc_slave=0x%x",read_eeprom_data);
	}
	if(error)
	{
		CAM_ERR(CAM_EEPROM, "\r\n chenjun=>read_eeprom_store error");
	}
	else
	{
		CAM_ERR(CAM_EEPROM, "\r\n chenjun=>read_eeprom_store ok");
	}
	
	return count;
}

static DEVICE_ATTR(read_eeprom, 0664, NULL, read_eeprom_show);
static ssize_t write_eeprom_store_master(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
	int error;
	uint32_t i;
#ifdef CHECK_SUM_CRC
	uint32_t crc_check_sum;
#endif
	uint32_t reg_addr=START_REGADDR_M;
	uint8_t *buffer_master;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];

	buffer_master = vmalloc(FILE_DATA_SIZE);
	recalibration_file_rw(RECALIBRATION_FILE_NAME_MASTER,buffer_master,0,FILE_DATA_SIZE);
	usleep_range(5000,5000);
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_M;
	eeprom->io_master_info.cci_client->i2c_freq_mode = 1;
	eeprom->io_master_info.master_type = CCI_MASTER;

	error = camera_io_init(&(eeprom->io_master_info));
	if (error) {
		CAM_ERR(CAM_EEPROM, "\r\n chenjun=>write_eeprom_store cci_init failed");
		return -EINVAL;
	}
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size = 1;
	/*close eeprom write protect*/
	i2c_reg_setting.reg_setting[0].reg_addr = 0x8000;
	i2c_reg_setting.reg_setting[0].reg_data =  0x0;
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun  guanbi baohu %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	for(i = 0 ; i < FILE_DATA_SIZE; i++){
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_master[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if(error <0){
		CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
		}

		usleep_range(5000,5000);;//delay 
	}
	
#ifdef CHECK_SUM_CRC
	//add crc_check_sum
	//crc_check_sum=0xFFFF;
	crc_check_sum=GetCRCChecksum(2048,buffer_master);
	CAM_ERR(CAM_EEPROM, "\r\n GetCRCChecksum  Failed :%d",crc_check_sum);	
	i2c_reg_setting.reg_setting[0].reg_addr = 0x08C6;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)((crc_check_sum&0xFF00)>>8);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	i2c_reg_setting.reg_setting[0].reg_addr = 0x08C7;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)(crc_check_sum&0x00FF);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	pr_err(" crc_check_sum =%d",crc_check_sum);
#endif
	
	/*open eeprom write protect*/
	i2c_reg_setting.reg_setting[0].reg_addr = 0x8000;
	i2c_reg_setting.reg_setting[0].reg_data =  0x0E;
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun  dakai baohu %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	vfree(buffer_master);
	return count;
}

static DEVICE_ATTR(write_eeprom_master, 0664, NULL, write_eeprom_store_master);
static ssize_t write_eeprom_store_slave(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cam_eeprom_ctrl_t *eeprom = i2c_get_clientdata(client);
	int error;
	uint32_t i;
	//uint32_t k=0;
#ifdef CHECK_SUM_CRC
	uint32_t crc_check_sum;
#endif
	uint32_t reg_addr=START_REGADDR_S;
	uint8_t *buffer_slave;
	struct cam_sensor_i2c_reg_setting i2c_reg_setting;
	struct cam_sensor_i2c_reg_array   reg_write_setting[EEPROM_ONE_BURST_SIZE];
	
	CAM_ERR(CAM_EEPROM, "\r\n chenjun=>write_eeprom_store");
	buffer_slave = vmalloc(FILE_DATA_SIZE);
	recalibration_file_rw(RECALIBRATION_FILE_NAME_SLAVE,buffer_slave,0,FILE_DATA_SIZE);
	usleep_range(5000,5000);
	eeprom->io_master_info.cci_client->cci_i2c_master = 0;
	eeprom->io_master_info.cci_client->sid = SLAVE_ADDR_S;
	eeprom->io_master_info.cci_client->i2c_freq_mode = 1;
	eeprom->io_master_info.master_type = CCI_MASTER;
	
	error = camera_io_init(&(eeprom->io_master_info));
	if (error) {
		CAM_ERR(CAM_EEPROM, "\r\n chenjun=>write_eeprom_store cci_init failed");
		return -EINVAL;
	}
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.delay = 0;
	i2c_reg_setting.reg_setting = reg_write_setting;
	i2c_reg_setting.size =1 ;

	/*close eeprom write protect*/
	i2c_reg_setting.reg_setting[0].reg_addr = 0x8000;
	i2c_reg_setting.reg_setting[0].reg_data =  0x0;//buffer[i];
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun  guanbi baohu %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);

	for(i = 0 ; i < FILE_DATA_SIZE; i++){
		i2c_reg_setting.reg_setting[0].reg_addr = reg_addr++;
		i2c_reg_setting.reg_setting[0].reg_data = buffer_slave[i];
		i2c_reg_setting.reg_setting[0].delay  = 0;
		i2c_reg_setting.reg_setting[0].data_mask =0;

		error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
		if(error <0){
		CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
		}

		usleep_range(5000,5000);;//delay 
	}
#ifdef CHECK_SUM_CRC
	//add crc_check_sum
		//crc_check_sum=0xFFFF;
	crc_check_sum=GetCRCChecksum(2048,buffer_slave);
	CAM_ERR(CAM_EEPROM, "\r\n GetCRCChecksum  Failed :%d",crc_check_sum);
	i2c_reg_setting.reg_setting[0].reg_addr = 0x1BA5;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)((crc_check_sum&0xFF00)>>8);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	i2c_reg_setting.reg_setting[0].reg_addr = 0x1BA6;
	i2c_reg_setting.reg_setting[0].reg_data = (uint16_t)(crc_check_sum&0x00FF);
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	pr_err(" crc_check_sum =%d",crc_check_sum);
#endif
	
	/*open eeprom write protect*/
	i2c_reg_setting.reg_setting[0].reg_addr = 0x8000;
	i2c_reg_setting.reg_setting[0].reg_data =  0x0E;
	i2c_reg_setting.reg_setting[0].delay  = 0;
	i2c_reg_setting.reg_setting[0].data_mask =0;
	error = camera_io_dev_write(&(eeprom->io_master_info), &i2c_reg_setting);
	if(error <0){
	CAM_ERR(CAM_EEPROM, "\r\n chenjun  dakai baohu %s: Failed :%d",__func__,error);
	}
	usleep_range(5000,5000);
	vfree(buffer_slave);
	return count;
}

static DEVICE_ATTR(write_eeprom_slave, 0664, NULL, write_eeprom_store_slave);

static struct attribute *eeprom_attrs[] = {
	&dev_attr_read_eeprom.attr,
	&dev_attr_write_eeprom_master.attr,
	&dev_attr_write_eeprom_slave.attr,
	NULL
};

static const struct attribute_group eeprom_attr_group = {
	.attrs = eeprom_attrs,
};
#endif
/* Huaqin add for biaoding by chenjun at 2018/10/08 end*/
static int cam_eeprom_i2c_driver_probe(struct i2c_client *client,
	 const struct i2c_device_id *id)
{
	int                             rc = 0;
	struct cam_eeprom_ctrl_t       *e_ctrl = NULL;
	struct cam_eeprom_soc_private  *soc_private = NULL;
	struct cam_hw_soc_info         *soc_info = NULL;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CAM_ERR(CAM_EEPROM, "i2c_check_functionality failed");
		goto probe_failure;
	}

	e_ctrl = kzalloc(sizeof(*e_ctrl), GFP_KERNEL);
	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "kzalloc failed");
		rc = -ENOMEM;
		goto probe_failure;
	}

	soc_private = kzalloc(sizeof(*soc_private), GFP_KERNEL);
	if (!soc_private)
		goto ectrl_free;

	e_ctrl->soc_info.soc_private = soc_private;

	i2c_set_clientdata(client, e_ctrl);

	mutex_init(&(e_ctrl->eeprom_mutex));

	soc_info = &e_ctrl->soc_info;
	soc_info->dev = &client->dev;
	soc_info->dev_name = client->name;
	e_ctrl->io_master_info.master_type = I2C_MASTER;
	e_ctrl->io_master_info.client = client;
	e_ctrl->eeprom_device_type = MSM_CAMERA_I2C_DEVICE;
	e_ctrl->cal_data.mapdata = NULL;
	e_ctrl->cal_data.map = NULL;
	e_ctrl->userspace_probe = false;

	rc = cam_eeprom_parse_dt(e_ctrl);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: soc init rc %d", rc);
		goto free_soc;
	}

	rc = cam_eeprom_update_i2c_info(e_ctrl, &soc_private->i2c_info);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: to update i2c info rc %d", rc);
		goto free_soc;
	}

	rc = cam_eeprom_init_subdev(e_ctrl);
	if (rc)
		goto free_soc;

	if (soc_private->i2c_info.slave_addr != 0)
		e_ctrl->io_master_info.client->addr =
			soc_private->i2c_info.slave_addr;

	e_ctrl->bridge_intf.device_hdl = -1;
	e_ctrl->bridge_intf.ops.get_dev_info = NULL;
	e_ctrl->bridge_intf.ops.link_setup = NULL;
	e_ctrl->bridge_intf.ops.apply_req = NULL;
	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, e_ctrl);
	e_ctrl->cam_eeprom_state = CAM_EEPROM_INIT;

	return rc;
free_soc:
	kfree(soc_private);
ectrl_free:
	kfree(e_ctrl);
probe_failure:
	return rc;
}

static int cam_eeprom_i2c_driver_remove(struct i2c_client *client)
{
	int                             i;
	struct v4l2_subdev             *sd = i2c_get_clientdata(client);
	struct cam_eeprom_ctrl_t       *e_ctrl;
	struct cam_eeprom_soc_private  *soc_private;
	struct cam_hw_soc_info         *soc_info;

	if (!sd) {
		CAM_ERR(CAM_EEPROM, "Subdevice is NULL");
		return -EINVAL;
	}

	e_ctrl = (struct cam_eeprom_ctrl_t *)v4l2_get_subdevdata(sd);
	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "eeprom device is NULL");
		return -EINVAL;
	}

	soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	if (!soc_private) {
		CAM_ERR(CAM_EEPROM, "soc_info.soc_private is NULL");
		return -EINVAL;
	}

	soc_info = &e_ctrl->soc_info;
	for (i = 0; i < soc_info->num_clk; i++)
		devm_clk_put(soc_info->dev, soc_info->clk[i]);

	mutex_destroy(&(e_ctrl->eeprom_mutex));
	kfree(soc_private);
	kfree(e_ctrl->io_master_info.cci_client);
	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, NULL);
	kfree(e_ctrl);

	return 0;
}

static int cam_eeprom_spi_setup(struct spi_device *spi)
{
	struct cam_eeprom_ctrl_t       *e_ctrl = NULL;
	struct cam_hw_soc_info         *soc_info = NULL;
	struct cam_sensor_spi_client   *spi_client;
	struct cam_eeprom_soc_private  *eb_info;
	struct cam_sensor_power_ctrl_t *power_info = NULL;
	int                             rc = 0;

	e_ctrl = kzalloc(sizeof(*e_ctrl), GFP_KERNEL);
	if (!e_ctrl)
		return -ENOMEM;

	soc_info = &e_ctrl->soc_info;
	soc_info->dev = &spi->dev;
	soc_info->dev_name = spi->modalias;

	e_ctrl->v4l2_dev_str.ops = &cam_eeprom_subdev_ops;
	e_ctrl->userspace_probe = false;
	e_ctrl->cal_data.mapdata = NULL;
	e_ctrl->cal_data.map = NULL;

	spi_client = kzalloc(sizeof(*spi_client), GFP_KERNEL);
	if (!spi_client) {
		kfree(e_ctrl);
		return -ENOMEM;
	}

	eb_info = kzalloc(sizeof(*eb_info), GFP_KERNEL);
	if (!eb_info)
		goto spi_free;
	e_ctrl->soc_info.soc_private = eb_info;

	e_ctrl->eeprom_device_type = MSM_CAMERA_SPI_DEVICE;
	e_ctrl->io_master_info.spi_client = spi_client;
	e_ctrl->io_master_info.master_type = SPI_MASTER;
	spi_client->spi_master = spi;

	power_info = &eb_info->power_info;
	power_info->dev = &spi->dev;

	/* set spi instruction info */
	spi_client->retry_delay = 1;
	spi_client->retries = 0;

	/* Initialize mutex */
	mutex_init(&(e_ctrl->eeprom_mutex));

	rc = cam_eeprom_parse_dt(e_ctrl);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: spi soc init rc %d", rc);
		goto board_free;
	}

	rc = cam_eeprom_spi_parse_of(spi_client);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "Device tree parsing error");
		goto board_free;
	}

	rc = cam_eeprom_init_subdev(e_ctrl);
	if (rc)
		goto board_free;

	e_ctrl->bridge_intf.device_hdl = -1;
	e_ctrl->bridge_intf.ops.get_dev_info = NULL;
	e_ctrl->bridge_intf.ops.link_setup = NULL;
	e_ctrl->bridge_intf.ops.apply_req = NULL;

	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, e_ctrl);
	return rc;

board_free:
	kfree(e_ctrl->soc_info.soc_private);
spi_free:
	kfree(spi_client);
	kfree(e_ctrl);
	return rc;
}

static int cam_eeprom_spi_driver_probe(struct spi_device *spi)
{
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi_setup(spi);

	CAM_DBG(CAM_EEPROM, "irq[%d] cs[%x] CPHA[%x] CPOL[%x] CS_HIGH[%x]",
		spi->irq, spi->chip_select, (spi->mode & SPI_CPHA) ? 1 : 0,
		(spi->mode & SPI_CPOL) ? 1 : 0,
		(spi->mode & SPI_CS_HIGH) ? 1 : 0);
	CAM_DBG(CAM_EEPROM, "max_speed[%u]", spi->max_speed_hz);

	return cam_eeprom_spi_setup(spi);
}

static int cam_eeprom_spi_driver_remove(struct spi_device *sdev)
{
	int                             i;
	struct v4l2_subdev             *sd = spi_get_drvdata(sdev);
	struct cam_eeprom_ctrl_t       *e_ctrl;
	struct cam_eeprom_soc_private  *soc_private;
	struct cam_hw_soc_info         *soc_info;

	if (!sd) {
		CAM_ERR(CAM_EEPROM, "Subdevice is NULL");
		return -EINVAL;
	}

	e_ctrl = (struct cam_eeprom_ctrl_t *)v4l2_get_subdevdata(sd);
	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "eeprom device is NULL");
		return -EINVAL;
	}

	soc_info = &e_ctrl->soc_info;
	for (i = 0; i < soc_info->num_clk; i++)
		devm_clk_put(soc_info->dev, soc_info->clk[i]);

	kfree(e_ctrl->io_master_info.spi_client);
	soc_private =
		(struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;
	if (soc_private) {
		kfree(soc_private->power_info.gpio_num_info);
		kfree(soc_private);
	}
	mutex_destroy(&(e_ctrl->eeprom_mutex));
	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, NULL);
	kfree(e_ctrl);

	return 0;
}

static int32_t cam_eeprom_platform_driver_probe(
	struct platform_device *pdev)
{
	int32_t                         rc = 0;
	struct cam_eeprom_ctrl_t       *e_ctrl = NULL;
	struct cam_eeprom_soc_private  *soc_private = NULL;

	e_ctrl = kzalloc(sizeof(struct cam_eeprom_ctrl_t), GFP_KERNEL);
	if (!e_ctrl)
		return -ENOMEM;

	e_ctrl->soc_info.pdev = pdev;
	e_ctrl->soc_info.dev = &pdev->dev;
	e_ctrl->soc_info.dev_name = pdev->name;
	e_ctrl->eeprom_device_type = MSM_CAMERA_PLATFORM_DEVICE;
	e_ctrl->cal_data.mapdata = NULL;
	e_ctrl->cal_data.map = NULL;
	e_ctrl->userspace_probe = false;

	e_ctrl->io_master_info.master_type = CCI_MASTER;
	e_ctrl->io_master_info.cci_client = kzalloc(
		sizeof(struct cam_sensor_cci_client), GFP_KERNEL);
	if (!e_ctrl->io_master_info.cci_client) {
		rc = -ENOMEM;
		goto free_e_ctrl;
	}

	soc_private = kzalloc(sizeof(struct cam_eeprom_soc_private),
		GFP_KERNEL);
	if (!soc_private) {
		rc = -ENOMEM;
		goto free_cci_client;
	}
	e_ctrl->soc_info.soc_private = soc_private;
	soc_private->power_info.dev = &pdev->dev;

	/* Initialize mutex */
	mutex_init(&(e_ctrl->eeprom_mutex));
	rc = cam_eeprom_parse_dt(e_ctrl);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: soc init rc %d", rc);
		goto free_soc;
	}
	rc = cam_eeprom_update_i2c_info(e_ctrl, &soc_private->i2c_info);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "failed: to update i2c info rc %d", rc);
		goto free_soc;
	}

	rc = cam_eeprom_init_subdev(e_ctrl);
	if (rc)
		goto free_soc;

	e_ctrl->bridge_intf.device_hdl = -1;
	e_ctrl->bridge_intf.ops.get_dev_info = NULL;
	e_ctrl->bridge_intf.ops.link_setup = NULL;
	e_ctrl->bridge_intf.ops.apply_req = NULL;

	platform_set_drvdata(pdev, e_ctrl);
	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, e_ctrl);

	e_ctrl->cam_eeprom_state = CAM_EEPROM_INIT;
/* Huaqin add for biaoding by chenjun at 2018/10/08 start*/
#ifdef huaqin_calibration
	rc = sysfs_create_group(&pdev->dev.kobj, &eeprom_attr_group);
	if (rc) {
		CAM_ERR(CAM_EEPROM, "\r\n chenjun Failure create eeprom sysfs group , error: %d\n",
			rc);
		goto free_soc;
	}
#endif
/* Huaqin add for biaoding by chenjun at 2018/10/08 end*/
	return rc;
free_soc:
	kfree(soc_private);
free_cci_client:
	kfree(e_ctrl->io_master_info.cci_client);
free_e_ctrl:
	kfree(e_ctrl);
	return rc;
}

static int cam_eeprom_platform_driver_remove(struct platform_device *pdev)
{
	int                        i;
	struct cam_eeprom_ctrl_t  *e_ctrl;
	struct cam_hw_soc_info    *soc_info;

	e_ctrl = platform_get_drvdata(pdev);
	if (!e_ctrl) {
		CAM_ERR(CAM_EEPROM, "eeprom device is NULL");
		return -EINVAL;
	}

	soc_info = &e_ctrl->soc_info;

	for (i = 0; i < soc_info->num_clk; i++)
		devm_clk_put(soc_info->dev, soc_info->clk[i]);

	mutex_destroy(&(e_ctrl->eeprom_mutex));
	kfree(soc_info->soc_private);
	kfree(e_ctrl->io_master_info.cci_client);
	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(&e_ctrl->v4l2_dev_str.sd, NULL);
	kfree(e_ctrl);
	return 0;
}

static const struct of_device_id cam_eeprom_dt_match[] = {
	{ .compatible = "qcom,eeprom" },
	{ }
};


MODULE_DEVICE_TABLE(of, cam_eeprom_dt_match);

static struct platform_driver cam_eeprom_platform_driver = {
	.driver = {
		.name = "qcom,eeprom",
		.owner = THIS_MODULE,
		.of_match_table = cam_eeprom_dt_match,
		.suppress_bind_attrs = true,
	},
	.probe = cam_eeprom_platform_driver_probe,
	.remove = cam_eeprom_platform_driver_remove,
};

static const struct i2c_device_id cam_eeprom_i2c_id[] = {
	{ "msm_eeprom", (kernel_ulong_t)NULL},
	{ }
};

static struct i2c_driver cam_eeprom_i2c_driver = {
	.id_table = cam_eeprom_i2c_id,
	.probe  = cam_eeprom_i2c_driver_probe,
	.remove = cam_eeprom_i2c_driver_remove,
	.driver = {
		.name = "msm_eeprom",
	},
};

static struct spi_driver cam_eeprom_spi_driver = {
	.driver = {
		.name = "qcom_eeprom",
		.owner = THIS_MODULE,
		.of_match_table = cam_eeprom_dt_match,
	},
	.probe = cam_eeprom_spi_driver_probe,
	.remove = cam_eeprom_spi_driver_remove,
};
static int __init cam_eeprom_driver_init(void)
{
	int rc = 0;

	rc = platform_driver_register(&cam_eeprom_platform_driver);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "platform_driver_register failed rc = %d",
			rc);
		return rc;
	}

	rc = spi_register_driver(&cam_eeprom_spi_driver);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "spi_register_driver failed rc = %d", rc);
		return rc;
	}

	rc = i2c_add_driver(&cam_eeprom_i2c_driver);
	if (rc < 0) {
		CAM_ERR(CAM_EEPROM, "i2c_add_driver failed rc = %d", rc);
		return rc;
	}

	return rc;
}

static void __exit cam_eeprom_driver_exit(void)
{
	platform_driver_unregister(&cam_eeprom_platform_driver);
	spi_unregister_driver(&cam_eeprom_spi_driver);
	i2c_del_driver(&cam_eeprom_i2c_driver);
}

module_init(cam_eeprom_driver_init);
module_exit(cam_eeprom_driver_exit);
MODULE_DESCRIPTION("CAM EEPROM driver");
MODULE_LICENSE("GPL v2");
