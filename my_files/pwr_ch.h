#ifndef _PWR_CH_H
#define _PWR_CH_H

#include "sys_uart.h"
#include "my_gpio.h"
#include "debug.h"

//режим канала
#define PWR_CH_OFF					(0x00)
#define PWR_CH_ON		 			(0x01)

//типы ошибки питания
#define PWR_CH_ERR_NO_ERR			(0x00)
#define PWR_CH_ERR_VOLTAGE			(0x01 << 0)
#define PWR_CH_ERR_PWR				(0x01 << 1)
#define PWR_CH_ERR_SWITCH			(0x01 << 2)
#define PWR_CH_ERR_OTHER			(0x01 << 3)

#define PWR_CH_STATUS_MASK			(0xFF)  //! для определения длины переменной под статус
#define PWR_CH_STATUS_RELEASE		(0)
#define PWR_CH_STATUS_SET			(1)

// параметры аппаратной части
#define PWR_CH_I_R_SHUNT_OHM		(0.05)
#define PWR_CH_I_ADM4073_K			(20.)

//границы питания по умолчанию
#define PWR_CH_BOUNDS_DEFAULT		{5.5, 4.5, 3.6, 3.0, 5.0, 0.5} //! 5v0_max, 5v0_min, 3v3_max, 3v3_min, pwr_max, pwr_min

//значение таймаута для проверки ошибок при переключении
#define PWR_CH_ERROR_CHECK_TIMEOUT_MS	(3000)

#pragma pack(push, 2)
/**
  * @brief  структура контроллера питания
  */
typedef struct
{ 
	uint16_t IPwr5V; 		// +0
	uint16_t UPwr5V; 		// +2
	uint16_t U3V3; 			// +4
	uint16_t U5V; 			// +6
	uint8_t Id; 			// +8
	uint8_t PwrOn; 			// +9
} type_MIM_PWR_STRUCT; 		// 10

typedef struct
{
	uint8_t i2c_addr;
	//
	type_MIM_PWR_STRUCT pwr_info;
	type_SysUART* suart_ptr;
	float max_5v0, min_5v0, max_3v3, min_3v3;
	float power_max, power_min;
	//
	float u_5v_pwr;
	float i_5v_pwr;
	float u_5v_self;
	float u_3v3_self;
	uint8_t mode_rd; //! прочитанный режим
	//
	uint8_t mode;
	uint8_t error, error_old;
	uint8_t error_cnt;
	uint8_t error_check_inh;
	uint32_t error_check_timeout_ms;
} type_PWR_CHANNEL;

#pragma pack(pop)

int8_t pwr_ch_init(type_PWR_CHANNEL* pwr_ch_ptr, type_SysUART* suart_ptr, uint8_t addr);
void pwr_ch_process(type_PWR_CHANNEL* pwr_ch_ptr, uint32_t period_ms);
void pwr_ch_set_bound(type_PWR_CHANNEL* pwr_ch_ptr, float max_5v0, float min_5v0, float max_3v3, float min_3v3, float pow_max, float pow_min);
uint8_t pwr_ch_get_error(type_PWR_CHANNEL* pwr_ch_ptr, uint8_t *error);
void pwr_ch_on_off(type_PWR_CHANNEL* pwr_ch_ptr, uint8_t mode); //работает для всех каналов, кроме 0-го (МС) - он всегда включен
void pwr_ch_get_data(type_PWR_CHANNEL* pwr_ch_ptr);
int8_t pwr_ch_check_rd_data(type_PWR_CHANNEL* pwr_ch_ptr);
void pwr_ch_update_pwr_data(type_PWR_CHANNEL* pwr_ch_ptr);
void  pwr_ch_error_collector(type_PWR_CHANNEL* pwr_ch_ptr, uint32_t flag, uint8_t type);
#endif
