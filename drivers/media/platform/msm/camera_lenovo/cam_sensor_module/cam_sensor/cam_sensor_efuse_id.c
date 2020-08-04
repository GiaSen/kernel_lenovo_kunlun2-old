
#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"

#include "cam_sensor_efuse_id.h"

/* #define USE_MY_DBG
#define USE_MY_ERR
#include <linux/yzm_dbg.h>
#undef FILE_NAME
#define  FILE_NAME  "cam_sensor_efuse_id.c"
 */
extern char front_cam_name[64];// s5k3p9
extern char frontaux_cam_name[64];//  s5k5e9
extern char back_cam_name[64];//  ov16e10
extern char backaux_cam_name[64];// ov8856


extern char front_cam_efuse_id[17];// s5k3p9
extern char frontaux_cam_efuse_id[17];// s5k5e9
extern char back_cam_efuse_id[33];//  ov16e10
extern char backaux_cam_efuse_id[33];// ov8856


static void cam_sensor_UpdateSettings(
uint32_t reg_addr,
enum camera_sensor_i2c_type addr_type,
uint32_t reg_data,
enum camera_sensor_i2c_type data_type,
struct cam_sensor_i2c_reg_setting  *i2c_reg_settings)
{
	struct cam_sensor_i2c_reg_array    *i2c_reg_array = i2c_reg_settings->reg_setting;
	
	i2c_reg_settings->size = 1;
	i2c_reg_settings->delay = 0;
	i2c_reg_settings->addr_type = addr_type;
	i2c_reg_settings->data_type = data_type;

	i2c_reg_array->reg_addr = reg_addr;
	i2c_reg_array->reg_data = reg_data;

}


//static int32_t get_efuse_id_s5k5e9(struct cam_sensor_ctrl_t *s_ctrl) 
static int32_t get_efuse_id_s5k(struct cam_sensor_ctrl_t *s_ctrl, char * pstr_efuse_id) 
{

    char hex_efuseid;
    char str_efuseid[17]={0};
    uint8_t dummy;
    uint8_t i;
    
    struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
    struct cam_sensor_i2c_reg_array    i2c_reg_array;
    
    
    i2c_reg_settings.reg_setting = &i2c_reg_array;
    
    WRITE_OTP_ADDR(0x0100, 0x01)// Streaming ON
    msleep(50);
    WRITE_OTP_ADDR(0x0A02, 0)  // Write OTP Page
    WRITE_OTP_ADDR(0x0A00, 0x01)// Write Read CMD
    msleep(1);
    READ_OTP_ADDR(0x0A01, (uint32_t *)&dummy)// Complete =  Read CMD Result   1
    //CAM_DBG(CAM_SENSOR,"Read CMD Result=%d   ",dummy);

    // page0   Read Chip ID
    for(i=0; i<8; i++)
    {
        READ_OTP_ADDR(0x0A04 + i, (uint32_t *)&hex_efuseid)
        //CAM_DBG(CAM_SENSOR, "i=%d   hex_efuseid=0x%x ", i, hex_efuseid);
        sprintf(str_efuseid+2*i,"%02x", hex_efuseid);
    }
    //CAM_DBG(CAM_SENSOR, "str_efuseid=%s", str_efuseid);

    strcpy(pstr_efuse_id, str_efuseid); 
    CAM_ERR(CAM_SENSOR,"pstr_efuse_id=%s", pstr_efuse_id);
    
    
/* // 转换为 page 17，去读    
    WRITE_OTP_ADDR(0x0A02, 17)  // Write OTP Page
    WRITE_OTP_ADDR(0x0A00, 0x01)// Write Read CMD
    // page17  
    for(i=0; i<8; i++)
    {
        READ_OTP_ADDR(0x0A04 + i, &efuseid[i])        
        CAM_ERR(CAM_SENSOR,"s5k efuseid[%d]:0x%x ", i, efuseid[i]);
    }
 */
    

    WRITE_OTP_ADDR(0x0A00, 0x04)// Clear error bits
    WRITE_OTP_ADDR(0x0A00, 0x00)// Initial command

    return 0;
}



static int32_t get_efuse_id_ov(struct cam_sensor_ctrl_t *s_ctrl, char * pstr_efuse_id) 
{

    char hex_efuseid;
    char str_efuseid[33]={0};

    uint8_t i;
    
    struct cam_sensor_i2c_reg_setting  i2c_reg_settings;
    struct cam_sensor_i2c_reg_array    i2c_reg_array;
    
    
    i2c_reg_settings.reg_setting = &i2c_reg_array;
    
    WRITE_OTP_ADDR(0x0100, 0x01)
    WRITE_OTP_ADDR(0x3D84, 0x40)  
/*     WRITE_OTP_ADDR(0x3D88, 0x7000)
    WRITE_OTP_ADDR(0x3D8A, 0x700F) */
    WRITE_OTP_ADDR(0x3D88, 0x00)
    WRITE_OTP_ADDR(0x3D89, 0x70)
    WRITE_OTP_ADDR(0x3D8A, 0x0F)
    WRITE_OTP_ADDR(0x3D8B, 0x70)
    
    WRITE_OTP_ADDR(0x0100, 0x01)

    // Read Chip ID
    for(i=0; i<16; i++)
    {
        READ_OTP_ADDR(0x7000 + i, (uint32_t *)&hex_efuseid)
        //CAM_DBG(CAM_SENSOR, "i=%d   hex_efuseid=0x%x ", i, hex_efuseid);
        sprintf(str_efuseid+2*i,"%02x", hex_efuseid);
		msleep(1);
    }
    //CAM_DBG(CAM_SENSOR, "str_efuseid=%s", str_efuseid);

    strcpy(pstr_efuse_id, str_efuseid); 
    CAM_ERR(CAM_SENSOR,"pstr_efuse_id=%s", pstr_efuse_id);
    
    return 0;
}



int32_t get_CameName_and_EfuseId(struct cam_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint32_t sensor_id = 0;
    
    sensor_id = s_ctrl->sensordata->slave_info.sensor_id;
    
    switch(sensor_id){
        
        case SENSER_ID_s5k3p9: {
            CAM_DBG(CAM_SENSOR," case SENSER_ID_s5k3p9 ");
            strcpy(front_cam_name,"sunny_s5k3p9");
            get_efuse_id_s5k(s_ctrl, front_cam_efuse_id);
        }
        break;
        
        case SENSER_ID_s5k5e9: {
            CAM_DBG(CAM_SENSOR," case SENSER_ID_s5k5e9 ");
            strcpy(frontaux_cam_name,"qtech_s5k5e9");
            get_efuse_id_s5k(s_ctrl, frontaux_cam_efuse_id);
        }
        break;
        
        case SENSER_ID_ov16e10: {
            CAM_DBG(CAM_SENSOR," case SENSER_ID_ov16b10 ");
            strcpy(back_cam_name,"qtech_ov16b10");
            get_efuse_id_ov(s_ctrl, back_cam_efuse_id);
        }
        break;
    
        case SENSER_ID_ov8856: {
            CAM_DBG(CAM_SENSOR," case SENSER_ID_ov8856 ");
            strcpy(backaux_cam_name,"qtech_ov8856");
            get_efuse_id_ov(s_ctrl, backaux_cam_efuse_id);
        }
        break;
    
        default:
        {
            CAM_ERR(CAM_SENSOR, "sensor id error   %d", sensor_id);
            rc = -1;
        }
    }

    return rc;
    
}

