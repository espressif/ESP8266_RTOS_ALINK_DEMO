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

#include "user_light.h"
#if USER_PWM_LIGHT_EN
struct light_saved_param light_param;
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[PLUG_KEY_NUM];

uint32 pwmio_info[8][3]={ {PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
                                  {PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
                                  {PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
                                  {PWM_3_OUT_IO_MUX,PWM_3_OUT_IO_FUNC,PWM_3_OUT_IO_NUM},
                                  {PWM_4_OUT_IO_MUX,PWM_4_OUT_IO_FUNC,PWM_4_OUT_IO_NUM},
                                  };

/******************************************************************************
 * FunctionName : user_plug_short_press
 * Description  : key's short press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void  
user_key_short_press(void)
{
    printf("Button pressed\n");
}

/******************************************************************************
 * FunctionName : user_plug_long_press
 * Description  : key's long press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void  
user_key_long_press(void)
{
    int boot_flag=12345;
 //   user_esp_platform_set_active(0);
    system_restore();
    
    system_rtc_mem_write(70, &boot_flag, sizeof(boot_flag));
    system_rtc_mem_read(70, &boot_flag, sizeof(boot_flag));

#if RESTORE_KEEP_TIMER
    user_platform_timer_bkup();
#endif 

    system_restart();
}

/******************************************************************************
 * FunctionName : user_get_key_status
 * Description  : a
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
BOOL  
user_get_key_status(void)
{
    return get_key_status(single_key[0]);
}

/******************************************************************************
 * FunctionName : user_light_get_duty
 * Description  : get duty of each channel
 * Parameters   : uint8 channel : LIGHT_RED/LIGHT_GREEN/LIGHT_BLUE
 * Returns      : NONE
*******************************************************************************/
uint32  
user_light_get_duty(uint8 channel)
{
    return light_param.pwm_duty[channel];
}

/******************************************************************************
 * FunctionName : user_light_set_duty
 * Description  : set each channel's duty params
 * Parameters   : uint8 duty    : 0 ~ PWM_DEPTH
 *                uint8 channel : LIGHT_RED/LIGHT_GREEN/LIGHT_BLUE
 * Returns      : NONE
*******************************************************************************/
void  
user_light_set_duty(uint32 duty, uint8 channel)
{
    if (duty != light_param.pwm_duty[channel]) {
        pwm_set_duty(duty, channel);

        light_param.pwm_duty[channel] = pwm_get_duty(channel);
    }
}

/******************************************************************************
 * FunctionName : user_light_get_period
 * Description  : get pwm period
 * Parameters   : NONE
 * Returns      : uint32 : pwm period
*******************************************************************************/
uint32  
user_light_get_period(void)
{
    return light_param.pwm_period;
}

/******************************************************************************
 * FunctionName : user_light_set_duty
 * Description  : set pwm frequency
 * Parameters   : uint16 freq : 100hz typically
 * Returns      : NONE
*******************************************************************************/
void  
user_light_set_period(uint32 period)
{
    if (period != light_param.pwm_period) {
        pwm_set_period(period);

        light_param.pwm_period = pwm_get_period();
    }
}
void  
user_light_restart(void)
{
    spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
    spi_flash_write((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
                (uint32 *)&light_param, sizeof(struct light_saved_param));

	pwm_start();
}

/******************************************************************************
 * FunctionName : user_light_init
 * Description  : light demo init, mainy init pwm
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void  user_light_init(void)
{
    /*init to off*/
    uint32 pwm_duty_init[PWM_CHANNEL];
    light_param.pwm_period = 1000;
    memset(pwm_duty_init,0,PWM_CHANNEL*sizeof(uint32));
    pwm_init(light_param.pwm_period, pwm_duty_init,PWM_CHANNEL,pwmio_info); 

    /*set target valuve from memory*/
    spi_flash_read((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,(uint32 *)&light_param, sizeof(struct light_saved_param));
    if(light_param.pwm_period>10000 || light_param.pwm_period <1000){
            light_param.pwm_period = 1000;
            light_param.pwm_duty[0]= APP_MAX_PWM;
            light_param.pwm_duty[1]= APP_MAX_PWM;
            light_param.pwm_duty[2]= APP_MAX_PWM;
            light_param.pwm_duty[3]= APP_MAX_PWM;
            light_param.pwm_duty[4]= APP_MAX_PWM;
    }
    printf("LIGHT P:%d",light_param.pwm_period);
    printf(" R:%d",light_param.pwm_duty[LIGHT_RED]);
    printf(" G:%d",light_param.pwm_duty[LIGHT_GREEN]);
    printf(" B:%d",light_param.pwm_duty[LIGHT_BLUE]);
    if(PWM_CHANNEL>LIGHT_COLD_WHITE){
        printf(" CW:%d",light_param.pwm_duty[LIGHT_COLD_WHITE]);
        printf(" WW:%d\r\n",light_param.pwm_duty[LIGHT_WARM_WHITE]);
    }else{
        printf("\r\n");
    }

    light_set_aim(light_param.pwm_duty[LIGHT_RED],
                    light_param.pwm_duty[LIGHT_GREEN],
                    light_param.pwm_duty[LIGHT_BLUE], 
                    light_param.pwm_duty[LIGHT_COLD_WHITE],
                    light_param.pwm_duty[LIGHT_WARM_WHITE],
                    light_param.pwm_period);
        
	return;
}



void user_esp_pwm_test_task(void *pvParameters)
{
	uint32 r,g,b,cw,ww,period;

	r = 0;
	g = 0;
	b = 0;
	cw = 0;

	period = 1000;

	while(1)   // a demo test pwm
	{
		ESP_DBG(("*****test****r:%d,g:%d,b:%d,cw:%d,ww:%d",r,g,b,cw,ww));
        light_set_aim(r,g,b,cw,ww,period);

		r+=5;
		g+=5;
		b+=5;
		cw+=5;
		ww+=5;
		
		if(r>100)
			r = 0;
		if(g>100)
			g = 0;
		if(b>100)
			b = 0;
		if(cw>100)
			cw = 0;
		if(ww>100)
			ww = 0;
		vTaskDelay(50);
	}
	vTaskDelete(NULL);
	return;
}


void user_esp_test_pwm(void)
{
	user_light_init();
//	xTaskCreate(user_esp_pwm_test_task, "pwm_task_test", 256, NULL, 5, NULL);
	return;
}

#endif

