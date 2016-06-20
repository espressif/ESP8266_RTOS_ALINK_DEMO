/*
* Copyright (c) 2014-2015 Alibaba Group. All rights reserved.
*
* Alibaba Group retains all right, title and interest (including all
* intellectual property rights) in and to this computer program, which is
* protected by applicable intellectual property laws.  Unless you have
* obtained a separate written license from Alibaba Group., you are not
* authorized to utilize all or a part of this computer program for any
* purpose (including reproduction, distribution, modification, and
* compilation into object code), and you must immediately destroy or
* return to Alibaba Group all copies of this computer program.  If you
* are licensed by Alibaba Group, your rights to utilize this computer
* program are limited by the terms of that license.  To obtain a license,
* please contact Alibaba Group.
*
* This computer program contains trade secrets owned by Alibaba Group.
* and, unless unauthorized by Alibaba Group in writing, you agree to
* maintain the confidentiality of this computer program and related
* information and to not disclose this computer program and related
* information to any other person or entity.
*
* THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND
* Alibaba Group EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
* INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
* PURPOSE, TITLE, AND NONINFRINGEMENT.
*/
#include "c_types.h"
#include "alink_export.h"
#include "alink_json.h"
#include <stdio.h>
#include <string.h>
#include "alink_export_rawdata.h"
#include "esp_common.h"
#include "user_config.h"
#if USER_UART_CTRL_DEV_EN
#include "user_uart.h" // user uart handler head
#endif
#if USER_PWM_LIGHT_EN
#include "user_light.h"  // user pwm light head
#endif
#define wsf_deb  os_printf
#define wsf_err os_printf
//#define PASS_THROUGH 
//#define SUB_DEV_ENABLE // 子设备功能sample 未实现
/* 设备信息：根据网页注册信息导出的电子表格更新对应信息 */
/* device info */
#define DEV_NAME "ALINKTEST"
#define DEV_CATEGORY "LIVING"
#define DEV_TYPE "LIGHT"
#ifdef PASS_THROUGH
#define DEV_MODEL "ALINKTEST_LIVING_LIGHT_SMARTLED_LUA"
#define ALINK_KEY "bIjq3G1NcgjSfF9uSeK2"
#define ALINK_SECRET "W6tXrtzgQHGZqksvJLMdCPArmkecBAdcr2F5tjuF"
#else
#define DEV_MODEL "ALINKTEST_LIVING_LIGHT_SMARTLED"
#define ALINK_KEY "ljB6vqoLzmP8fGkE6pon"
#define ALINK_SECRET "YJJZjytOCXDhtQqip4EjWbhR95zTgI92RVjzjyZF"
#endif
#define DEV_MANUFACTURE "ALINKTEST"
/*sandbox key/secret*/
#define ALINK_KEY_SANDBOX "dpZZEpm9eBfqzK7yVeLq"
#define ALINK_SECRET_SANDBOX "THnfRRsU5vu6g6m9X6uFyAjUWflgZ0iyGjdEneKm"
/*设备硬件信息:系统上电后读取的硬件/固件信息,此处为演示需要,直接定义为宏.产品对接时,需要调用自身接口获取*/
#define DEV_SN "1234567890"
#define DEV_VERSION "1.0.0;APP2.0;OTA1.0"
#define DEV_MAC "19:FE:34:A2:C7:1A"	//"AA:CC:CC:CA:CA:01" // need get from device
#define DEV_CHIPID "3D0044000F47333139373030"	// need get from device
/*alink-sdk 信息 */

extern void alink_sleep(int);
/*do your job here*/
/*这里是一个虚拟的设备,将5个设备属性对应的值保存到全局变量,真实的设备需要去按照实际业务处理这些属性值 */

VIRTUAL_DEV virtual_device;// = {0x01, 0x30, 0x50, 0, 0x01};

//const char *main_dev_params =
  //  "{\"attrSet\": [ \"OnOff_Power\", \"Color_Temperature\", \"Light_Brightness\", \"TimeDelay_PowerOff\", \"WorkMode_MasterLight\"], \"OnOff_Power\": { \"value\": \"%d\" }, \"Color_Temperature\": { \"value\": \"%d\" }, \"Light_Brightness\": { \"value\": \"%d\" }, \"TimeDelay_PowerOff\": { \"value\": \"%d\"}, \"WorkMode_MasterLight\": { \"value\": \"%d\"}}";
char *device_attr[5] = { "OnOff_Power", "Color_Temperature", "Light_Brightness",
	"TimeDelay_PowerOff", "WorkMode_MasterLight"
};   // this is a demo json package data, real device need to update this package

const char *main_dev_params =
    "{\"OnOff_Power\": { \"value\": \"%d\" }, \"Color_Temperature\": { \"value\": \"%d\" }, \"Light_Brightness\": { \"value\": \"%d\" }, \"TimeDelay_PowerOff\": { \"value\": \"%d\"}, \"WorkMode_MasterLight\": { \"value\": \"%d\"}}";

char device_status_change = 1;
/*设备上报数据,需要客户根据具体业务去实现*/

#define buffer_size 512
static int ICACHE_FLASH_ATTR alink_device_post_data(alink_down_cmd_ptr down_cmd)
{
	alink_up_cmd up_cmd;
	int ret = ALINK_ERR;
	//char buffer[1024];
	char *buffer = NULL;
//      static int count=0;
	if (device_status_change) {

/*		count++;
		if(count>20)
		{

			device_status_change=0;
			wsf_deb("alink_device_post_raw_data skip");
			return 0;
		}*/

		wsf_deb("##[%s][%s|%d]Malloc %u. Available memory:%d.\n", __FILE__, __FUNCTION__, __LINE__,
			buffer_size, system_get_free_heap_size());

		buffer = (char *)malloc(buffer_size);
		if (buffer == NULL)
			return -1;

		memset(buffer, 0, buffer_size);

		sprintf(buffer, main_dev_params, virtual_device.power,
			virtual_device.temp_value, virtual_device.light_value,
			virtual_device.time_delay, virtual_device.work_mode);
		up_cmd.param = buffer;
		if (down_cmd != NULL) {
			up_cmd.target = down_cmd->account;
			up_cmd.resp_id = down_cmd->id;
		} else {
			up_cmd.target = NULL;
			up_cmd.resp_id = -1;
		}
		ret = alink_post_device_data(&up_cmd);

		if (ret == ALINK_ERR) {
			wsf_err("post failed!\n");
			alink_sleep(2000);
		} else {
			wsf_deb("dev post data success!\n");
			device_status_change = 0;
		}

		if (buffer)
			free(buffer);
		wsf_deb("##[%s][%s][%d]  Free |Aviable Memory|%5d| \n", __FILE__, __FUNCTION__, __LINE__,
			system_get_free_heap_size());

		stack_free_size();
	}
	return ret;

}

/* do your job end */

int sample_running = ALINK_TRUE;

/*get json cmd from server 服务器下发命令,需要设备端根据具体业务设定去解析处理*/
int ICACHE_FLASH_ATTR main_dev_set_device_status_callback(alink_down_cmd_ptr down_cmd)
{

	json_value *jptr;
	json_value *jstr;
	json_value *jstr_value;
	int value = 0, i = 0;
	char *value_str = NULL;

	wsf_deb("%s %d \n",__FUNCTION__,__LINE__);
	wsf_deb("%s %d\n%s\n",down_cmd->uuid,down_cmd->method, down_cmd->param);

	jptr = json_parse(down_cmd->param, strlen(down_cmd->param));
#if USER_UART_CTRL_DEV_EN
	for (i = 0; i < 5; i++) 
	{
		jstr = json_object_object_get_e(jptr, device_attr[i]);
		jstr_value = json_object_object_get_e(jstr, "value");
		value_str = json_object_to_json_string_e(jstr_value);
		
		if (value_str) {
			value = atoi(value_str);
			cus_wifi_handler_alinkdata2mcu(i, value);

			ESP_DBG(("power:0x%X,temp_value:0x%X,light_value:0x%X,time_delay:0x%X,woke_mode:0x%X",virtual_device.power,\
				virtual_device.temp_value,virtual_device.light_value,virtual_device.time_delay,virtual_device.work_mode));
		}
	}
#endif
#if USER_PWM_LIGHT_EN
	USER_LIGHT_DATA *user_light_data_ptr = zalloc(sizeof(USER_LIGHT_DATA));
	// Set r g b w data,Developers need to parse the real device parameters corresponding to the Json package
	light_set_aim(user_light_data_ptr->light_r,user_light_data_ptr->light_g,user_light_data_ptr->light_b,user_light_data_ptr->light_cw,user_light_data_ptr->light_ww,user_light_data_ptr->light_period);

	free(user_light_data_ptr);
#endif
#if USER_VIRTUAL_DEV_TEST
	for (i = 0; i < 5; i++) 
	{
		jstr = json_object_object_get_e(jptr, device_attr[i]);
		jstr_value = json_object_object_get_e(jstr, "value");
		value_str = json_object_to_json_string_e(jstr_value);
		if (value_str) {
			value = atoi(value_str);
			switch (i) {
			case 0:
				if (virtual_device.power != value) {
					virtual_device.power = value;
				}
				break;
			case 1:
				if (virtual_device.temp_value != value) {
					virtual_device.temp_value = value;
				}
				break;
			case 2:
				if (virtual_device.light_value != value) {
					virtual_device.light_value = value;
				}
				break;
			case 3:
				if (virtual_device.time_delay != value) {
					virtual_device.time_delay = value;
				}
				break;
			case 4:
				if (virtual_device.work_mode != value) {
					virtual_device.work_mode = value;
				}
				break;
			default:
				break;
			}
		}
	}
#endif

	json_value_free(jptr);
	device_status_change = 1;   // event send current real device status to the alink server

	return 0;		// alink_device_post_data(down_cmd);
	/* do your job end! */
}

/*服务器查询设备状态,需要设备上报状态*/
int ICACHE_FLASH_ATTR main_dev_get_device_status_callback(alink_down_cmd_ptr down_cmd)
{
	wsf_deb("%s %d \n", __FUNCTION__, __LINE__);
	wsf_deb("%s %d\n%s\n", down_cmd->uuid, down_cmd->method, down_cmd->param);
	device_status_change = 1;

	return 0;		//alink_device_post_data(down_cmd);
}

/* 根据不同系统打印剩余内存,用于平台调试 */
int ICACHE_FLASH_ATTR print_mem_callback(void *a, void *b)
{
	int ret = 0;
	ret = system_get_free_heap_size();
	os_printf("heap_size %d\n", ret);
	return ret;
}

#ifdef PASS_THROUGH
/* device response server command,用户需要自己实现这个函数,处理服务器下发的指令*/
/* this sample save cmd value to virtual_device*/
static int ICACHE_FLASH_ATTR execute_cmd(const char *rawdata, int len)
{
	int ret = 0, i = 0;
	if (len < 1)
		ret = -1;
	for (i = 0; i < len; i++) {
		wsf_deb("%2x ", rawdata[i]);
		switch (i) {
		case 2:
			if (virtual_device.power != rawdata[i]) {
				virtual_device.power = rawdata[i];
			}
			break;
		case 4:
			if (virtual_device.temp_value != rawdata[i]) {
				virtual_device.temp_value = rawdata[i];
			}
			break;
		case 5:
			if (virtual_device.light_value != rawdata[i]) {
				virtual_device.light_value = rawdata[i];
			}
/*			// for test alink unbind
			if (virtual_device.light_value == 0x5a) {
				wsf_deb("test for alink unbind\n");
				virtual_device.light_value = 0x59;
				alink_unbind();
			} else
				wsf_deb("0x%2x", virtual_device.light_value);
*/
			break;
		case 6:
			if (virtual_device.time_delay != rawdata[i]) {
				virtual_device.time_delay = rawdata[i];
			}
			break;
		case 3:
			if (virtual_device.work_mode != rawdata[i]) {
				virtual_device.work_mode = rawdata[i];
			}
			break;
		default:
			break;
		}
	}
	return ret;
}

/*获取设备信息,需要用户实现 */
static int ICACHE_FLASH_ATTR get_device_status(char *rawdata, int len)
{
	/* do your job here */
	int ret = 0;
	if (len > 7) {
		rawdata[0] = 0xaa;
		rawdata[1] = 0x07;
		rawdata[2] = virtual_device.power;
		rawdata[3] = virtual_device.work_mode;
		rawdata[4] = virtual_device.temp_value;
		rawdata[5] = virtual_device.light_value;
		rawdata[6] = virtual_device.time_delay;
		rawdata[7] = 0x55;
	} else {
		ret = -1;
	}
	/* do your job end */
	return ret;
}

static unsigned int delta_time = 0;

/*主动上报设备状态,需要用户自己实现*/
int ICACHE_FLASH_ATTR alink_device_post_raw_data(void)
{
	/* do your job here */
	int len = 8, ret = 0;
	char rawdata[8] = { 0 };
	if (device_status_change) {
		wsf_deb("[%s][%d|  Available memory:%d.\n", __FILE__, __LINE__,system_get_free_heap_size());

		delta_time = system_get_time() - delta_time;
		wsf_deb("%s %d \n delta_time = %d ", __FUNCTION__, __LINE__, delta_time / 1000);
		get_device_status(rawdata, len);

		ret = alink_post_device_rawdata(rawdata, len);
		device_status_change = 0;

		if (ret) {
			wsf_err("post failed!\n");
		} else {
			wsf_deb("dev post raw data success!\n");
		}
	}
	/* do your job end */
	return ret;
}

/*透传方式服务器查询指令回调函数*/

int ICACHE_FLASH_ATTR rawdata_get_callback(const char *in_rawdata, int in_len, char *out_rawdata, int *out_len)
{
	//TODO: 下发指令到MCU
	int ret = 0;
	wsf_deb("%s %d \n", __FUNCTION__, __LINE__);
	//ret=alink_device_post_raw_data(); //  此例是假设能同步获取到虚拟设备数据, 实际应用中,处理服务器指令是异步方式,需要厂家处理完毕后主动上报一次设备状态
// do your job end!
	device_status_change = 1;

	return ret;
}

/*透传方式服务器下发指令回调函数*/
/*实际应用中,处理服务器指令是异步方式,需要厂家处理完毕后主动上报一次设备状态*/
int ICACHE_FLASH_ATTR rawdata_set_callback(char *rawdata, int len)
{
	// TODO: 
	//get cmd from server, do your job here!
	int ret = 0;
	wsf_deb("%s %d \n", __FUNCTION__, __LINE__);
	ret = execute_cmd(rawdata, len);
	//ret=alink_device_post_raw_data();
	// do your job end!
	delta_time = system_get_time();
	device_status_change = 1;
	return ret;
}

#endif //PASS_THROUGH

/*alink-sdk 状态查询回调函数*/
int ICACHE_FLASH_ATTR alink_handler_systemstates_callback(void *dev_mac, void *sys_state)
{
	char uuid[33] = { 0 };
	char *mac = (char *)dev_mac;
	enum ALINK_STATUS *state = (enum ALINK_STATUS *)sys_state;
	switch (*state) {
	case ALINK_STATUS_INITED:
		break;
	case ALINK_STATUS_REGISTERED:
		sprintf(uuid, "%s", alink_get_uuid(NULL));
		wsf_deb("ALINK_STATUS_REGISTERED, mac %s uuid %s \n", mac, uuid);
		break;
	case ALINK_STATUS_LOGGED:
		sprintf(uuid, "%s", alink_get_uuid(NULL));
		wsf_deb("ALINK_STATUS_LOGGED, mac %s uuid %s\n", mac, uuid);
		break;
	default:
		break;
	}
	return 0;
}

/* fill device info 这里设备信息需要修改对应宏定义,其中DEV_MAC和DEV_CHIPID 需要用户自己实现接口函数*/
void ICACHE_FLASH_ATTR alink_fill_deviceinfo(struct device_info *deviceinfo)
{
	uint8 macaddr[6];
	//fill main device info here
	strcpy(deviceinfo->name, DEV_NAME);
	strcpy(deviceinfo->sn, DEV_SN);
	strcpy(deviceinfo->key, ALINK_KEY);
	strcpy(deviceinfo->model, DEV_MODEL);
	strcpy(deviceinfo->secret, ALINK_SECRET);
	strcpy(deviceinfo->type, DEV_TYPE);
	strcpy(deviceinfo->version, DEV_VERSION);
	strcpy(deviceinfo->category, DEV_CATEGORY);
	strcpy(deviceinfo->manufacturer, DEV_MANUFACTURE);
	strcpy(deviceinfo->key_sandbox, ALINK_KEY_SANDBOX);
	strcpy(deviceinfo->secret_sandbox, ALINK_SECRET_SANDBOX);

	if (wifi_get_macaddr(0, macaddr)) {
		wsf_deb("macaddr=%02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(macaddr));
		snprintf(deviceinfo->mac, sizeof(deviceinfo->mac), "%02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(macaddr));
	} else
		strcpy(deviceinfo->mac, DEV_MAC);
//	if ((macaddr[4] == 0xc7) && (macaddr[5] == 0x18))	// the mac  18:fe:34:a2:c7:18   binding CHIPID  "3D0044000F47333139373030" 
//	{
//		strcpy(deviceinfo->cid, DEV_CHIPID);
//	} else {
		snprintf(deviceinfo->cid, sizeof(deviceinfo->cid), "%024d", system_get_chip_id());
//	}
	wsf_deb("DEV_MODEL:%s \n", DEV_MODEL);
}


//#define GET_ALIGN_STRING_LEN(str)    ((strlen(str) + 3) & ~3)
/*ALINK_CONFIG_LEN 2048, len need < ALINK_CONFIG_LEN */
static int ICACHE_FLASH_ATTR write_config(unsigned char *buffer, unsigned int len)
{
	int res = 0, pos = 0;

	if (buffer == NULL) {
		return ALINK_ERR;
	}
	if (len > ALINK_CONFIG_LEN)
		len = ALINK_CONFIG_LEN;

	res = spi_flash_erase_sector(LFILE_START_ADDR / 4096);	//one sector is 4KB 
	if (res != SPI_FLASH_RESULT_OK) {
		wsf_err("erase flash data fail\n");
	} else {
		wsf_err("erase flash data %d Byte\n", res);
	}
	os_printf("write data:\n");


	res = spi_flash_write((LFILE_START_ADDR), (uint32 *) buffer, len);
	if (res != SPI_FLASH_RESULT_OK) {
		wsf_err("write data error\n");
		return ALINK_ERR;
	}
	wsf_deb("write key(%s) success.", buffer);
	return ALINK_OK;
}

 /*ALINK_CONFIG_LEN 2048, len need < ALINK_CONFIG_LEN */
static int ICACHE_FLASH_ATTR read_config(unsigned char *buffer, unsigned int len)
{

	int res = 0;
	int pos = 0;
	res = spi_flash_read(LFILE_START_ADDR, (uint32 *) buffer, len);
	if (res != SPI_FLASH_RESULT_OK) {
		wsf_err("read flash data error\n");
		return ALINK_ERR;
	}
	os_printf("read data:\n");
	return ALINK_OK;
}

int ICACHE_FLASH_ATTR alink_get_debuginfo(info_type type, char *status)
{
	int used;  
	switch (type) {    
		case MEMUSED:    
			used = 100 - ((system_get_free_heap_size()*100)/(96*1024));   
			sprintf(status, "%d%%", used);    
			break;    
		case WIFISTRENGTH:    
			sprintf(status , "%ddB",wifi_station_get_rssi());    
			break;    
		default:    
			status[0] = '\0';    
			break;  
	}  
	return 0;
}
int esp_ota_firmware_update( char * buffer, int len)
{
    os_printf("esp_ota_firmware_update \n");
   return upgrade_download(buffer , len);
}

int esp_ota_upgrade(void)
{
    os_printf("esp_ota_upgrade \n");
    system_upgrade_reboot();
    return 0;
}
extern int need_notify_app;
extern int  need_factory_reset ;

void set_thread_stack_size(struct thread_stacksize * p_thread_stacksize)
{
    p_thread_stacksize->alink_main_thread_size = 0xc00;
    p_thread_stacksize->send_work_thread_size = 0x800;
    p_thread_stacksize->wsf_thread_size = 0x1000;
    p_thread_stacksize->func_thread_size = 0x800;
}
   
int ICACHE_FLASH_ATTR alink_demo()
{
	struct device_info main_dev;
	
	memset(&main_dev, 0, sizeof(main_dev));
	alink_fill_deviceinfo(&main_dev);	// 必须根据PRD表格更新设备信息
	alink_set_loglevel(ALINK_LL_DEBUG | ALINK_LL_INFO | ALINK_LL_WARN | ALINK_LL_ERROR);
	//alink_set_loglevel(ALINK_LL_ERROR);
	//alink_enable_sandbox_mode();                                      // 线上环境需要注解此函数
	main_dev.sys_callback[ALINK_FUNC_SERVER_STATUS] = alink_handler_systemstates_callback;
	alink_set_callback(ALINK_FUNC_AVAILABLE_MEMORY, print_mem_callback);

	/* ALINK_CONFIG_LEN 2048 */
	alink_register_cb(ALINK_FUNC_READ_CONFIG, (void *)&read_config);
	alink_register_cb(ALINK_FUNC_WRITE_CONFIG, (void *)&write_config);
	alink_register_cb(ALINK_FUNC_GET_STATUS, alink_get_debuginfo);
	//alink_enable_sandbox(&main_dev);                                      // 线上环境需要注解此函数
    alink_register_cb(ALINK_FUNC_OTA_FIRMWARE_SAVE, esp_ota_firmware_update);
    alink_register_cb(ALINK_FUNC_OTA_UPGRADE, esp_ota_upgrade);
	/*start alink-sdk */
    set_thread_stack_size(&g_thread_stacksize);
    #ifdef PASS_THROUGH		
	alink_start_rawdata(&main_dev, rawdata_get_callback, rawdata_set_callback);
#else // 非透传方式(设备与服务器采用json格式数据通讯)
	main_dev.dev_callback[ACB_GET_DEVICE_STATUS] = main_dev_get_device_status_callback;
	main_dev.dev_callback[ACB_SET_DEVICE_STATUS] = main_dev_set_device_status_callback;
	
	alink_start(&main_dev);	//register main device here
#endif //PASS_THROUGH

	os_printf("%s %d wait time=%d \n", __FUNCTION__, __LINE__, ALINK_WAIT_FOREVER);

	ESP_DBG(("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
	if(ALINK_OK == alink_wait_connect(NULL, ALINK_WAIT_FOREVER))	//wait main device login, -1 means wait forever
	{
#if USER_UART_CTRL_DEV_EN
		char send_buf_alink_connOK[]={0x31, 0x31, 0x31, 0x31}; // demo data to tell uart mcu dev, alink conn success
		uart0_write_data(send_buf_alink_connOK,sizeof(send_buf_alink_connOK));
#endif
	}
	else
	{
#if USER_UART_CTRL_DEV_EN
		char send_buf_alink_connFailed[]={0x32, 0x32, 0x32, 0x32}; // demo data to tell uart mcu dev, alink conn success
		uart0_write_data(send_buf_alink_connFailed,sizeof(send_buf_alink_connFailed));
#endif
	}
	if(need_notify_app) {
		need_notify_app = 0;
		uint8 macaddr[6];
		char mac[17+1];
		if (wifi_get_macaddr(0, macaddr)) {
			os_printf("macaddr=%02x:%02x:%02x:%02x:%02x:%02x\n", MAC2STR(macaddr));
			snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(macaddr));
			zconfig_notify_app(DEV_MODEL, mac, ""); // if not factory reset , 
		}
	}
	//printf("%s %d \n",__FUNCTION__,__LINE__);

	//printf("alink_demo heap_size %d\n",system_get_free_heap_size());
	//system_print_meminfo();


	/* 设备主动上报数据 */
	while (sample_running) {

		//os_printf("%s %d \n",__FUNCTION__,__LINE__);
#ifdef PASS_THROUGH
		alink_device_post_raw_data();
#else
		alink_device_post_data(NULL);
#endif //PASS_THROUGH

		if(need_factory_reset) {
		wsf_deb("key to factory_reset\n");
			need_factory_reset = 0;
		alink_factory_reset();
		}
		alink_sleep(1000);
	}

	/*  设备退出alink-sdk */
	alink_end();

	return 0;
}
