#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include "rtc.h"
#include "stdarg.h"
#include "usart.h"
#include "gpio.h"

#define DBG_GPIO_PORT 		{GPIOE, GPIOE, 	GPIOE}
#define DBG_GPIO_LINE 		{2, 	3, 	    4}

// #define DEBUG

void printf_time(void);
void printf_buff(uint8_t *buff, uint8_t len, char end_char);



#endif
