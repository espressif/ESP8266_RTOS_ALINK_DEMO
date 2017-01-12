/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "alink_export.h"
#include "user_config.h"
#include "esp_ota.h"
#include "esp_system.h"
int dbg_get_recv_times = 0;

#if USER_UART_CTRL_DEV_EN
#include "user_uart.h" // user uart handler head
#endif
#if USER_PWM_LIGHT_EN
#include "user_light.h"  // user pwm light head
#endif
#define ENABLE_GPIO_KEY

#ifdef ENABLE_GPIO_KEY
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/key.h"
#include "espressif/esp8266/eagle_soc.h"
struct single_key_param *single_key[2];
struct keys_param keys;
static int is_long_press = 0;


#endif	

int aws_sample(void);
static char vendor_ssid[32];
static char vendor_passwd[64];
static char *vendor_tpsk = "PLnBaCPHF7icf65a5nJmcL2GZC+w3vwCnH36k8O91og=";
int ICACHE_FLASH_ATTR vendor_callback(char *ssid, char *passwd, char *bssid, unsigned int security, char channel) 
{
	if (!ssid) {
		os_printf("zconfig timeout!\n");
		system_restart();
	} else {
		os_printf("ssid:%s \n", ssid);
		struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
		strncpy(config->ssid, ssid, 32);
		strncpy(config->password, passwd, 64);
		wifi_station_set_config(config);
		free(config);
		wifi_station_connect();
		
	} 
} 

unsigned portBASE_TYPE ICACHE_FLASH_ATTR stack_free_size() 
{
	xTaskHandle taskhandle;
	unsigned portBASE_TYPE stack_freesize = 0;
	taskhandle = xTaskGetCurrentTaskHandle();
	stack_freesize = uxTaskGetStackHighWaterMark(taskhandle);
	os_printf("stack free size =%u\n", stack_freesize);
	return stack_freesize;
}

void ICACHE_FLASH_ATTR startdemo_task(void *pvParameters) 
{
	os_printf("start demo task heap_size %d\n", system_get_free_heap_size());
	stack_free_size();
	
	while (1) {
		int ret = wifi_station_get_connect_status();   // wait for sys wifi connected OK.
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);	 // 100 ms
	}
	
	alink_demo();
	vTaskDelete(NULL);
}

int need_notify_app = 0;
#if 1
void ICACHE_FLASH_ATTR wificonnect_task(void *pvParameters) 
{
	os_printf("enter smartconfig confing net mode\n");
	int ret=0;

	#if 1
	ret=aws_sample();   // alink smart config start
	if(ret==-1){// alink smarconfig err,enter softap config net mode
		ESP_DBG(("********ENTER SOFTAP MODE******"));
		os_printf("enter softap config net mode\n");
		aws_softap_main();
	}
	#endif
	
	while(1) {
		// device could show smart config status here, such as light Flashing.
		int ret = wifi_station_get_connect_status();
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(200 / portTICK_RATE_MS);
	}
	need_notify_app = 1;
	vTaskDelete(NULL);
}
#else
void ICACHE_FLASH_ATTR wificonnect_task(void *pvParameters)  // it is a test router cfg info for wifi conncted
{
    signed char ret;
    wifi_set_opmode(STATION_MODE);
    struct ip_info sta_ipconfig;
    struct station_config config;
    bzero(&config, sizeof(struct station_config));
    sprintf(config.ssid, "IOT_DEMO_TEST");
    sprintf(config.password, "123456789");
    wifi_station_set_config(&config);
    wifi_station_connect();
    wifi_get_ip_info(STATION_IF, &sta_ipconfig);
    while(sta_ipconfig.ip.addr == 0 ) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        wifi_get_ip_info(STATION_IF, &sta_ipconfig);
    }
    vTaskDelete(NULL);
}

#endif
 
#if  0				// wifi
    //wifi_set_event_handler_cb(wifi_event_hand_function);
static void ICACHE_FLASH_ATTR wifi_event_hand_function(System_Event_t * event) 
{
	os_printf("\n****wifi_event_hand_function %d******\n", event->event_id);
	switch (event->event_id) {
	case EVENT_STAMODE_CONNECTED:
		break;
	case EVENT_STAMODE_DISCONNECTED:
		os_printf(" \n EVENT_STAMODE_DISCONNECTED \n");
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		break;
	case EVENT_STAMODE_GOT_IP:
		os_printf(" \n EVENT_STAMODE_GOT_IP \n 0x%x\n", event->event_info.got_ip.ip.addr);
		
		    //alink_demo(); 
		    break;
	case EVENT_STAMODE_DHCP_TIMEOUT:
		break;
	default:
		break;
	}
}

 
#endif	
#ifdef ENABLE_GPIO_KEY
void ICACHE_FLASH_ATTR setLed1State(int flag) 
{
	if (0 == flag) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED1_GPIO_NO), 1);	// led 0ff
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED1_GPIO_NO), 0);	// led on
	}
}

void ICACHE_FLASH_ATTR setLed2State(int flag) 
{
	if (0 == flag) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED2_GPIO_NO), 0);	// led off
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED2_GPIO_NO), 1);	// led on
	}
}

void ICACHE_FLASH_ATTR led_gpio_init(void) 
{
	GPIO_ConfigTypeDef pGPIOConfig;
	pGPIOConfig.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
	pGPIOConfig.GPIO_Pullup = GPIO_PullUp_EN;
	pGPIOConfig.GPIO_Mode = GPIO_Mode_Output;
	pGPIOConfig.GPIO_Pin = USER_CFG_STATUS_LED1_GPIO_PIN;	//led1
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED1_GPIO_NO), 1);	//close led1
	pGPIOConfig.GPIO_Pin = USER_CFG_STATUS_LED2_GPIO_PIN;	//led2
	gpio_config(&pGPIOConfig);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(USER_CFG_STATUS_LED2_GPIO_NO), 0);	//close led2
	setLed1State(0);
	setLed2State(0);
} 

int need_factory_reset = 0;
static int sw1_long_key_flag = 0;
static int sw2_long_key_flag = 0;
void ICACHE_FLASH_ATTR user_key_long_press(void) 
{
	
    ESP_DBG((" long press.."));
	os_printf("[%s][%d] user key long press \n\r",__FUNCTION__,__LINE__);
	sw1_long_key_flag = 1;
	setLed1State(1);
	setLed2State(0);
	need_factory_reset = 1;
	os_printf("need_factory_reset =%d \n", need_factory_reset);

	return;
} 

void ICACHE_FLASH_ATTR user_key_short_press(void) 
{
	ESP_DBG((" short press.."));

	int ret = 0;
       if(sw1_long_key_flag) {
           sw1_long_key_flag = 0;
           return ;
       }
	os_printf("[%s][%d] user key press \n\r",__FUNCTION__,__LINE__);
	setLed1State(0);
	setLed2State(1);
	setSmartConfigFlag(0x1);
	system_restart();
} 

#define MD5_STR_LEN 32
#define MAX_URL_LEN 256
typedef struct
{
    char fwName[256];
    char fwVersion[256];
    unsigned int fwSize;
    char fwUrl[MAX_URL_LEN];
    char fwMd5[MD5_STR_LEN+1];
    int zip;
}FwFileInfo_t;
typedef xTaskHandle pthread_mutex_t;      /* identify a mutex */
typedef xTaskHandle pthread_t;            /* identify a thread */

typedef struct
{
    int status;
    pthread_mutex_t mutex;
    pthread_t id;
}FwOtaStatus_t;

extern FwOtaStatus_t fwOtaStatus;
extern FwFileInfo_t fwFileInfo;
extern void dumpFwInfo();
extern void alink_ota_main_thread(FwFileInfo_t * pFwFileInfo);
extern void alink_ota_init();
#define pthread_mutex_init(a, b)sys_mutex_new(a)

void ota_test()
{
    pthread_mutex_init(&fwOtaStatus.mutex,NULL);
    alink_ota_init();
    strcpy(fwFileInfo.fwMd5,"B3822931C345C88C9BA4AFB03C0C7295");
    fwFileInfo.fwSize = 388388;
    //strcpy(fwFileInfo.fwUrl,"http://otalink.alicdn.com/ALINKTEST_LIVING_LIGHT_SMARTLED_LUA/1.0.1/user1.2048.new.5.bin");
    strcpy(fwFileInfo.fwUrl,"http://otalink.alicdn.com/ALINKTEST_LIVING_LIGHT_SMARTLED/1.0.1/user1.2048.new.5.bin");
    strcpy(fwFileInfo.fwVersion,"1.0.1");
    fwFileInfo.zip = 0;
    dumpFwInfo();
    create_thread(&fwOtaStatus.id, "firmware upgrade pthread", (void *)alink_ota_main_thread, &fwFileInfo, 0xc00);

}
void ICACHE_FLASH_ATTR sw2_key_long_press() 
{
	os_printf("sw2_key_long_press\n");
    sw2_long_key_flag = 1;
    setLed1State(0);
    setLed2State(0);
}

void ICACHE_FLASH_ATTR sw2_key_short_press() 
{
	if (sw2_long_key_flag) {
		sw2_long_key_flag = 0;
		return ;
    }
    os_printf("sw2_key short \n");
	//ota_test();
}

void ICACHE_FLASH_ATTR reset_keyinit(void) 
{
	single_key[0] =
	key_init_single(13, PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, user_key_long_press, user_key_short_press);
	single_key[1] =
	key_init_single(4 , PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, sw2_key_long_press, sw2_key_short_press);
	keys.key_num = 2;
	keys.single_key = single_key;
	key_init(&keys);
} 

void ICACHE_FLASH_ATTR init_key() 
{
	led_gpio_init();
//	reset_keyinit();
	user_key_input_init(USER_CFG_KEY_GPIO_PIN);

	return;
} 

int ICACHE_FLASH_ATTR readSmartConfigFlag() {
	int res = 0;
	uint32  value = 0;
	res = spi_flash_read(LFILE_START_ADDR+LFILE_SIZE, &value, 4);
	if (res != SPI_FLASH_RESULT_OK) {
		os_printf("read flash data error\n");
		return ALINK_ERR;
	}
	os_printf("read data:0x%2x \n",value);
	return (int)value;
}
// flash写接口需要先擦后写,这里擦了4KB,会导致整个4KB flash存储空间的浪费,可以将这个变量合并保存到用户数据中.
int ICACHE_FLASH_ATTR setSmartConfigFlag(int value) 
{
		int res = 0;	
		uint32 data = (uint32) value;
		spi_flash_erase_sector((LFILE_START_ADDR+LFILE_SIZE)/4096);
		res = spi_flash_write((LFILE_START_ADDR+LFILE_SIZE), &data, 4);
		if (res != SPI_FLASH_RESULT_OK) {
			os_printf("write data error\n");
			return ALINK_ERR;
		}
		os_printf("write flag(%d) success.", value);
		return ALINK_OK;
}


#endif
void ICACHE_FLASH_ATTR wificonnect_test_conn_ap(void) 
{
    signed char ret;
	ESP_DBG(("set a test conn router."));
    wifi_set_opmode(STATION_MODE);
    struct ip_info sta_ipconfig;
    struct station_config config;
    bzero(&config, sizeof(struct station_config));
    sprintf(config.ssid, "IOT_DEMO_TEST");
    sprintf(config.password, "123456789");
    wifi_station_set_config(&config);
    wifi_station_connect();
    wifi_get_ip_info(STATION_IF, &sta_ipconfig);
	return;
}

static void ICACHE_FLASH_ATTR sys_show_rst_info(void)
{
	struct rst_info *rtc_info = system_get_rst_info();	
	os_printf("reset reason: %x\n", rtc_info->reason);	
	if ((rtc_info->reason == REASON_WDT_RST) ||	(rtc_info->reason == REASON_EXCEPTION_RST) ||(rtc_info->reason == REASON_SOFT_WDT_RST)) 
	{		
		if (rtc_info->reason == REASON_EXCEPTION_RST) 
		{			
			os_printf("Fatal exception (%d):\n", rtc_info->exccause);		
		}		
		os_printf("esp@bill dbg: epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);	
	}

	return;
}
void ICACHE_FLASH_ATTR user_check_rst_info()
{
    struct rst_info *rtc_info = system_get_rst_info();

    ESP_DBG(("reset reason: %x\n", rtc_info->reason));

	if ((rtc_info->reason == REASON_WDT_RST) || (rtc_info->reason == REASON_EXCEPTION_RST) ||(rtc_info->reason == REASON_SOFT_WDT_RST)) 
	{
        if (rtc_info->reason == REASON_EXCEPTION_RST) 
		{
            os_printf("Fatal exception (%d):\n", rtc_info->exccause);
        }
        os_printf(" esp dbg epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
    }
    else{
      ESP_DBG(("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
                rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc));  
    }

}
void ICACHE_FLASH_ATTR user_demo(void) 
{
	unsigned int ret = 0;
	os_printf("\n****************************\n");
	os_printf("****************************\n");
	os_printf("SDK version:%s\n,Alink version:%s\n,user fw ver:1.0.0(20170112@esp)\n", system_get_sdk_version(), USER_ALINK_GLOBAL_VER);
	os_printf("esp debug ESP@0112version heap_size %d\n", system_get_free_heap_size());
	os_printf("****************************\n");
	os_printf("****************************\n");
 //   user_check_rst_info();
#ifdef ENABLE_GPIO_KEY   // demo for smartplug class products
	init_key();
//	wifi_set_event_handler_cb(wifi_event_hand_function);
#endif	
	wifi_set_opmode(STATION_MODE);
	os_printf("##[%s][%s|%d]Malloc %u.Available memory:%d.\n", __FILE__, __FUNCTION__, __LINE__, \
		sizeof(struct station_config), system_get_free_heap_size());
	ret = readSmartConfigFlag();	// -1 read flash fail!
	os_printf(" read flag:%d \n", ret);
	if(ret >0) {		
		setSmartConfigFlag(0);	 // clear smart config flag
		xTaskHandle xIdleTaskHandle_alit;
	//	xTaskCreate(wificonnect_task, "wificonnect_task", 256+128, NULL, 2, NULL);
		xTaskCreate(wificonnect_task, "wificonnect_task", 256+128, NULL, 2, &xIdleTaskHandle_alit);

		ESP_DBG(("wifi conn task thandler:0x%x",xIdleTaskHandle_alit));		
//		need_notify_app = 1;
	}

#if 0
	while (1) {
		int ret = wifi_station_get_connect_status();   // wait for sys wifi connected OK.
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);	 // 100 ms
	}
	
#endif

	
	xTaskCreate(startdemo_task, "startdemo_task",(256*4), NULL, 2, NULL);
	
#if USER_PWM_LIGHT_EN
	user_esp_test_pwm();  // this a test ctrl pwm data, real device not need this
#endif
#if USER_UART_CTRL_DEV_EN
	user_uart_dev_start();  // create a task to handle uart data
#endif

	return;
}

LOCAL uint32 totallength = 0;
LOCAL uint32 sumlength = 0;
extern FwFileInfo_t fwFileInfo;

int upgrade_download(char *pusrdata, unsigned short length)
{
    char *ptr = NULL;
    char *ptmp2 = NULL;
    char lengthbuffer[32] ={0};
    bool ret = false;
    os_printf("%s %d totallength =%d length = %d \n",__FUNCTION__,__LINE__,totallength,length);
    if( (pusrdata == NULL )||length < 1) {
	 system_upgrade_flag_set(UPGRADE_FLAG_START);
	 sumlength = fwFileInfo.fwSize;
	 return -1;
    }
    if(sumlength < 1)
        return -1;
    if(totallength == 0){  // the first data packet
      
        ptr = pusrdata;
        if ((char)*(ptr) != 0xEA)
        {
            totallength = 0;
            sumlength = 0;
            return -1;
        }
        ret = ota_write_flash(pusrdata, length,true);
	 if(ret != false) {
            totallength += length;
	 }else {
            return -1;
	 }
    } else {
        totallength += length;
        if (totallength  > sumlength ){
            length = length - (totallength - sumlength);
            totallength = sumlength;
        }
        ret = ota_write_flash(pusrdata, length,true);
	 if (ret != true) {
	 	return -1; 
	 }
    }
    if (totallength == sumlength) {
        if( system_get_fw_start_sec() != 0 ) {
            if (-4 == upgrade_crc_check(system_get_fw_start_sec(),sumlength)) {
                totallength = 0;
                sumlength = 0;
	         return -1;
            } else {
                system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
                totallength = 0;
                sumlength = 0;
            }
        } else {
            totallength = 0;
            sumlength = 0;
        }    
    }
    return 0;
}
