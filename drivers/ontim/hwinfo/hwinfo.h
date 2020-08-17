#ifndef KEYWORD

#define KEYWORD_ENUM
#define KEYWORD(symbol) symbol,

enum HWINFO_E{
#endif

KEYWORD(CPU_TYPE)
KEYWORD(NFC_MFR)
KEYWORD(pon_reason)
KEYWORD(secboot_version)
KEYWORD(board_id)
KEYWORD(qcn_type)
KEYWORD(serialno)
KEYWORD(emmc_mfr)
KEYWORD(emmc_capacity)
KEYWORD(lpddr_mfr)
KEYWORD(lpddr_capacity)
KEYWORD(SPEAKER_MFR)    //Speaker box mfr
KEYWORD(FP_MFR)         //Fingerprint mfr
KEYWORD(LCD_MFR)        //LCD mfr
KEYWORD(TP_MFR)         //Touch mfr
KEYWORD(BATTARY_MFR)    //battary  mfr
KEYWORD(BATTARY_VOL)    //battary  voltage_now
KEYWORD(BATTARY_CAP)    //battary  capacity
KEYWORD(ADB_SN)         //adb sn
KEYWORD(TYPEC_MFR)      //Typec  mfr
KEYWORD(TYPEC_CC_STATUS)      //Typec  mfr
KEYWORD(BACK_CAM_MFR)         //camera  mfr
KEYWORD(BACKAUX_CAM_MFR)      //camera  mfr
KEYWORD(FRONT_CAM_MFR)        //camera  mfr
KEYWORD(FRONTAUX_CAM_MFR)        //camera  mfr
KEYWORD(BACK_CAM_EFUSE)         //camera  efuse
KEYWORD(BACKAUX_CAM_EFUSE)      //camera  efuse
KEYWORD(FRONT_CAM_EFUSE)        //camera  efuse
KEYWORD(FRONTAUX_CAM_EFUSE)        //camera  efuse
KEYWORD(CARD_HOLDER_PRESENT)  //card hold detect
KEYWORD(CHARGER_IC_MFR) //houzn add

#ifdef KEYWORD_ENUM
KEYWORD(HWINFO_MAX)
};
int smartisan_hwinfo_register(enum HWINFO_E e_hwinfo,char *hwinfo_name);
#undef KEYWORD_ENUM
#undef KEYWORD

#endif
