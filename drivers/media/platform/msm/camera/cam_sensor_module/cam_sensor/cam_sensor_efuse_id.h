
#ifndef _CAM_SENSOR_EFUSE_ID_H_
#define _CAM_SENSOR_EFUSE_ID_H_

/*                                       otp_sid    sid
#define SENSER_ID_ov16e10  0x1645        0xa0       0x20
#define SENSER_ID_s5k3p9   0x3109        0xA0       0x20
#define SENSER_ID_s5k5e9   0x559B        0x5A       0x5A
#define SENSER_ID_ov8856   0x885A        0xa2       0x6c
 */


#define SENSER_ID_s5k3p9   0x3109
#define SENSER_ID_ov16e10  0x1645
#define SENSER_ID_ov8856   0x885A
#define SENSER_ID_s5k5e9   0x559B

#define  WRITE_OTP_ADDR(reg_addr, reg_data)\
    cam_sensor_UpdateSettings(reg_addr, CAMERA_SENSOR_I2C_TYPE_WORD, \
    reg_data, CAMERA_SENSOR_I2C_TYPE_BYTE, &i2c_reg_settings);\
    camera_io_dev_write(&s_ctrl->io_master_info, &i2c_reg_settings);


#define  READ_OTP_ADDR(reg_addr, pu32_buf)\
    camera_io_dev_read(&s_ctrl->io_master_info, reg_addr, pu32_buf, \
    CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);




int32_t get_CameName_and_EfuseId(struct cam_sensor_ctrl_t *s_ctrl);


#endif


