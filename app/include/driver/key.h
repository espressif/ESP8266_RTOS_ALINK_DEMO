#ifndef __KEY_H__
#define __KEY_H__

#include "gpio.h"

typedef void (* key_function)(void);

#define USER_CFG_KEY_GPIO_NO  			12
#define USER_CFG_KEY_GPIO_PIN  			GPIO_Pin_12


#define USER_CFG_STATUS_LED1_GPIO_NO 	14
#define USER_CFG_STATUS_LED1_GPIO_PIN 	GPIO_Pin_14

#define USER_CFG_STATUS_LED2_GPIO_NO 	15
#define USER_CFG_STATUS_LED2_GPIO_PIN 	GPIO_Pin_15


struct single_key_param {
    uint32 gpio_name;
    os_timer_t key_5s;
    os_timer_t key_50ms;
    key_function short_press;
    key_function long_press;
    
    uint8 key_level;
    uint8 gpio_id;
    uint8 gpio_func;
};

struct keys_param {
    struct single_key_param **single_key;
    uint8 key_num;
};

struct single_key_param *key_init_single(uint8 gpio_id, uint32 gpio_name, uint8 gpio_func, key_function long_press, key_function short_press);
BOOL get_key_status(struct single_key_param *single_key);
void key_init(struct keys_param *key);
void user_key_input_init(u16 user_gpio_id);
void ICACHE_FLASH_ATTR setLed1State(int flag); 

extern unsigned char key_inter_flag;

#endif
