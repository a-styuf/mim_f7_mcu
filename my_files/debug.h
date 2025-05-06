#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include "rtc.h"
#include "stdarg.h"
#include "usart.h"
#include "gpio.h"
	
#define USART_PRINT
#define DEBUG

#define DBG_GPIO_PORT 		{GPIOE, GPIOE, 	GPIOE}
#define DBG_GPIO_LINE 		{2, 	3, 	    4}

void printf_buff(uint8_t *buff, uint8_t len, char end_char);

#endif
