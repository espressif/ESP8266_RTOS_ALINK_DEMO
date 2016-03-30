/******************************************************************************
 * Copyright (C) 2014 -2016  Espressif System
 *
 * FileName: upgrade_lib.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 * 2016/1/24, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"
#include "lwip/mem.h"
#include "esp_ota.h"

struct upgrade_param {
    uint32 fw_bin_addr;
    uint16 fw_bin_sec;
    uint16 fw_bin_sec_num;
    uint16 fw_bin_sec_earse;
    uint8 extra;
    uint8 save[4];
    uint8 *buffer;
};

LOCAL struct upgrade_param *upgrade;
LOCAL bool init_flag = false ;


/******************************************************************************
 * FunctionName : user_upgrade_internal
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool  system_upgrade_internal(struct upgrade_param *upgrade, uint8 *data, uint16 len,bool erase_flag)
{
    bool ret = false;
    if(data == NULL || len == 0)
    {
        return true;
    }
    upgrade->buffer = (uint8 *)zalloc(len + upgrade->extra);
    if(upgrade->buffer == NULL ) {
        os_printf("%s %d %d \n",__FUNCTION__,__LINE__,system_get_free_heap_size());
	      return false;
    }
    memcpy(upgrade->buffer, upgrade->save, upgrade->extra);
    memcpy(upgrade->buffer + upgrade->extra, data, len);
    len += upgrade->extra;
    upgrade->extra = len & 0x03;
    len -= upgrade->extra;
    memcpy(upgrade->save, upgrade->buffer + len, upgrade->extra);

    do {
        if (upgrade->fw_bin_addr + len >= (upgrade->fw_bin_sec + upgrade->fw_bin_sec_num) * SPI_FLASH_SEC_SIZE) {
            break;
        }
        if (len > SPI_FLASH_SEC_SIZE) {

        } else {
            /* earse sector, just earse when first enter this zone */
            if (upgrade->fw_bin_sec_earse != (upgrade->fw_bin_addr + len) >> 12) {
                upgrade->fw_bin_sec_earse = (upgrade->fw_bin_addr + len) >> 12;
                if (erase_flag == true) {
		                spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
		            }
            }
        }
        if (spi_flash_write(upgrade->fw_bin_addr, (uint32 *)upgrade->buffer, len) != SPI_FLASH_RESULT_OK) {
            break;
        }
        ret = true;
        upgrade->fw_bin_addr += len;
    } while (0);
    free(upgrade->buffer);
    upgrade->buffer = NULL;
    return ret;
}


/******************************************************************************
 * FunctionName : system_get_fw_start_sec
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
uint16 system_get_fw_start_sec()
{
	if(upgrade != NULL) {
		return upgrade->fw_bin_sec;
	} else {
		return 0;
	}
}
/******************************************************************************
 * FunctionName : system_upgrade_init
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
void  
system_upgrade_init(void)
{
    init_flag = false;
    uint32 user_bin2_start,user_bin1_start;
    uint8 spi_size_map = system_get_flash_size_map();
    if (upgrade == NULL) {
        upgrade = (struct upgrade_param *)zalloc(sizeof(struct upgrade_param));
    }
    user_bin1_start = 1; 
    if (spi_size_map == FLASH_SIZE_8M_MAP_512_512 || 
    		spi_size_map ==FLASH_SIZE_16M_MAP_512_512 ||
    		spi_size_map ==FLASH_SIZE_32M_MAP_512_512){
    		user_bin2_start = 129;
    		upgrade->fw_bin_sec_num = 123;
    } else if(spi_size_map == FLASH_SIZE_16M_MAP_1024_1024 || 
    		spi_size_map == FLASH_SIZE_32M_MAP_1024_1024){
    		user_bin2_start = 257;
    		upgrade->fw_bin_sec_num = 251;
    } else {
    		user_bin2_start = 65;
    		upgrade->fw_bin_sec_num = 59;
    }
    upgrade->fw_bin_sec = (system_upgrade_userbin_check() == USER_BIN1) ? user_bin2_start : user_bin1_start;
    upgrade->fw_bin_addr = upgrade->fw_bin_sec * SPI_FLASH_SEC_SIZE;
    init_flag = true;
}

/******************************************************************************
 * FunctionName : user_upgrade
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
bool  
ota_write_flash(uint8 *data, uint16 len,bool erase_flag)
{
    bool ret;
    if (init_flag == false) {
        system_upgrade_init();
    }
    ret = system_upgrade_internal(upgrade, data, len, erase_flag);
    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade_deinit
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
void  
system_upgrade_recycle(void)
{
    if (upgrade != NULL) {
    	free(upgrade);
    	upgrade = NULL;
    	init_flag = false;
    	return;
    }else {
    	init_flag = false;
    	return;
    }
}
