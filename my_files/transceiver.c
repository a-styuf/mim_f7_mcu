/**
 * @file transceiver.c
 * @author your name (you@domain.com)
 * @brief модуль для обработки данных и настроек для приема передачи через модули АА
 * @version 0.1
 * @date 2023-11-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "transceiver.h"

/**
 * @brief 
 * 
 * @param radio_ptr 
 * @param pwr_ch 
 * @param mb_addr 
 * @param huart_req 
 * @param self_id 
 */
void radio_init(type_RADIO* radio_ptr, uint16_t self_id, type_PWR_CHANNEL* pwr_ch_ptr, uint16_t can_addr)
{
	// интерфейсы к аппаратной части
	radio_ptr->pwr_ch_ptr = pwr_ch_ptr;
	// общие настройки
	radio_ptr->self_id = self_id;
	radio_ptr->can_addr = can_addr;
	//
	radio_ptr->status = 0x00;
	radio_ptr->error_cnt = 0x00;
	// инициализация рабочих интервалов
	radio_intervals_init(radio_ptr);
	//
	memset((uint8_t*)&radio_ptr->report, 0xFE, sizeof(type_RADIO_Report));
	//
	radio_ptr->last_call_time_us = 0;
	radio_ptr->call_interval_us = 0;
}
