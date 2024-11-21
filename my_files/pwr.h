#ifndef _PWR_H
#define _PWR_H

#include "pwr_ch.h"
#include "sys_uart.h"
#include "task_planner.h"
#include "debug.h"

#define PWR_PROCESS_PERIOD 100

/**
  * @brief  список каналов питания
*/
enum pwr_ch_enum
{
	PWR_CH_IM, 
	PWR_CH_S0, 		PWR_CH_S1, 		PWR_CH_S2, 		PWR_CH_S3,
	PWR_CH_S4, 		PWR_CH_S5, 		PWR_CH_S6, 		PWR_CH_S7,
	PWR_CH_ISM0, 	PWR_CH_ISM1, 	PWR_CH_ISM2, 	PWR_CH_ISM3,
	PWR_CH_UKV, 
	PWR_CH_NUMBER
};

#define PWR_STATE_DEFAULT 	(0x3FFF)

#define PWR_I2C_ADDR	{1,	2, 3, 4, 5, 6, 7, 8, 9, 10,	11,	12, 13, 14}

#define PWR_5V0_MAX	 	{5.5, 5.5, 5.5, 5.5, 5.5, 5.5, 5.5, 5.5, 5.5, 5.5, 5.5,	5.5, 5.5, 5.5}
#define PWR_5V0_MIN 	{4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5, 4.5,	4.5, 4.5, 4.5}

#define PWR_3V3_MAX	 	{3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6, 3.6}
#define PWR_3V3_MIN 	{3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0}

#define PWR_MAX	 		{5.0, 20.0, 10.0, 20.0, 10.0, 20.0, 10.0, 20.0, 10.0, 20.0, 20.0, 20.0, 20.0, 20.0}
#define PWR_MIN 		{0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,	0.5, 0.5, 0.5}

//типы ошибки питания

#pragma pack(push, 2)

typedef struct
{
	uint16_t status;  					// +0
	uint16_t state;  					// +2
	uint16_t presence;  				// +4
	uint16_t voltage[PWR_CH_NUMBER];  	// +6
	uint16_t current[PWR_CH_NUMBER];  	// +34
} type_PWR_TMI_REPORT;  				// 62

typedef struct
{ 
	type_PWR_CHANNEL ch[PWR_CH_NUMBER];
	float u_5v_pwr_fp[PWR_CH_NUMBER];
	float i_5v_pwr_fp[PWR_CH_NUMBER];
	float u_5v_self_fp[PWR_CH_NUMBER];
	float u_3v3_self_fp[PWR_CH_NUMBER];
	type_SysUART* suart_ptr;
	//
	uint32_t ch_state;  //! состояние канала: включен из ПО или выключен
	uint32_t ch_status;  //! состояние канала: 1 - есть ошибка
	uint16_t ch_presence;  //! наличие платы с системным контроллером
	int8_t init_report;
	uint8_t ch_read_queue, ch_queue_mode;
	//! переменные для работы с планировщиком задач
	uint64_t last_call_time_us;
	uint64_t call_interval_us;
} type_PWR_CONTROL;

#pragma pack(pop)


int8_t pwr_init(type_PWR_CONTROL* pwr_ptr, type_SysUART* suart_ptr);
void pwr_process(type_PWR_CONTROL* pwr_ptr, uint32_t period_ms);
void pwr_state_reset(type_PWR_CONTROL* pwr_ptr);
void pwr_get_tmi_report(type_PWR_CONTROL* pwr_ptr, type_PWR_TMI_REPORT* tmi_report);

int8_t pwr_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);

#endif
