#ifndef _CLOCK_H
#define _CLOCK_H

#include "rtc.h"  // HAL
#include "xtime.h"
#include "debug.h"

#define CLOCK_START_YEAR 2000
#define CLOCK_UNIX_TIME_2000_1_1 946684800

void clock_init(void);
uint32_t clock_get_time_s(void);
uint32_t clock_get_unix_time_s(void);
void clock_set_time_s(uint32_t time_s);
void clock_set_time_s_and_ms(uint32_t time_s, uint16_t ms);
void clock_get_bcd_array(uint8_t *array);
void clock_get_bcd_array_with_fractional_part(uint8_t *array);

uint16_t _get_month_offset(uint8_t year, uint8_t month);

#endif
