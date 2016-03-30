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

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#define RESTORE_KEEP_TIMER 0

#define USE_OPTIMIZE_PRINTF
extern  int need_factory_reset ;
#define LFILE_START_ADDR  0x1f8000// flash size is 0x200000
#define LFILE_SIZE 4096

#define USER_UART_CTRL_DEV_EN 	 (0)   // support allink uart smart device
#define USER_PWM_LIGHT_EN        (0)   // support alink pwm light  
#define USER_VIRTUAL_DEV_TEST	 (1)

#define CUS_GLOBAL_VER "AlinkV1.0.02-2016xxxx"


#define ESP_DBG(a) os_printf("[dbg@%s,%d]",__FUNCTION__,__LINE__);os_printf a;os_printf("\r\n")   // for dbg print

typedef struct virtual_dev {   // real device update this Structure parameters
	u32 power;
	u32 temp_value;
	u32 light_value;
	u32 time_delay;
	u32 work_mode;
}VIRTUAL_DEV;// virtual_device = {
//0x01, 0x30, 0x50, 0, 0x01};

extern VIRTUAL_DEV virtual_device;


#endif

