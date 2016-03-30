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

#include "c_types.h"
#include <stdio.h>
#include <string.h>
#include "esp_common.h"
#include "user_uart.h"

//#include "user_light.h"

#if USER_UART_CTRL_DEV_EN

#include "../../../include/driver/uart.h"

xQueueHandle xQueueCusUart;

void debug_print_hex_data(char*buf, int len)
{
	int i = 0;
	printf("\n_____________[%d]__________\n",len);
	for(i=0;i<len;i++)
	{
		printf("%X ",*(buf+i));
	}
	printf("\n____________________________\n");
	return;
}	

int uart0_write_data(u8 *data, int len)
{
	int re_len = 0;
	int i = 0;
	for(i  = 0; i <len; i++)
	{
		uart0_write_char(*(data+i));
	}
	return i;
}

void ICACHE_FLASH_ATTR cus_wifi_handler_alinkdata2mcu(u8 dat_index, int dat_value)
{
	ESP_DBG(("data2mcu handler, index[%x],data_value[%x]",dat_index,dat_value));
	// here handler user own uart protocol...
	return;
}
static u8 ICACHE_FLASH_ATTR cus_uart_data_handle(char *dat_in, int in_len, char *dat_out)
{
	ESP_DBG(("uart data handler.."));
	debug_print_hex_data(dat_in, in_len);
#if 0  // test data 
	u32 r,g,b,cw,ww;
	u8 buf[24]={0};
	r = dat_in[0];
	g = dat_in[1];
	b = dat_in[2];
	cw = dat_in[3];
	ww = dat_in[4];
	sprintf(buf,"is:[%X] [%X] [%X] [%X] [%X]ok",r,g,b,cw,ww);
	light_set_aim(r,g,b,cw,ww,1000);
	uart0_write_data(buf,strlen(buf));   // send data to device uart mcu, device mcu will handle this data from alink server
#endif	
	return 0x00;
}

void ICACHE_FLASH_ATTR user_uart_task(void *pvParameters)
{
    CusUartIntrPtr uartptrData;
	u32 sys_time_value = system_get_time();
	char uart_beat_data[]={0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};  // test uart beat data "12345678"
	
    while(1)
	{
		if (xQueueReceive(xQueueCusUart, (void *)&uartptrData, (portTickType)500/*portMAX_DELAY*/)) // wait about 5sec 
		{
			ESP_DBG(("data uart recv.."));
			debug_print_hex_data(uartptrData.rx_buf,uartptrData.rx_len);

			if(uartptrData.rx_len>0x00){
				cus_uart_data_handle(uartptrData.rx_buf, uartptrData.rx_len,NULL);
			}
		}

		if((system_get_time()-sys_time_value)>=(60*1000*1000))  //about 1min, send data to uart0, demo beat data
		{
			ESP_DBG(("uart beat data.[%d][%d]",sys_time_value,system_get_time()));
			ESP_DBG(("heap_size %d\n", system_get_free_heap_size()));
			uart0_write_data(uart_beat_data,sizeof(uart_beat_data));
			sys_time_value = system_get_time();
		}
	}

    vTaskDelete(NULL);
	
}

void ICACHE_FLASH_ATTR user_uart_dev_start(void)
{
	uart_init_new();   // cfg uart0 connection device MCU, cfg uart1 TX debug output
    xQueueCusUart = xQueueCreate((unsigned portBASE_TYPE)CUS_UART0_QUEUE_LENGTH, sizeof(CusUartIntrPtr));
    xTaskCreate(user_uart_task, (uint8 const *)"uart", 256, NULL, tskIDLE_PRIORITY + 2, NULL);

	return;
}

#endif
