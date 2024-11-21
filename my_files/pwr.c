/**
  ******************************************************************************
  * @file           : pwr.c
  * @version        : v1.0
  * @brief          : библиотека для управления каналами питания МС
  * @author			: Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "pwr.h"

/**
  * @brief  инициализация состояния питания
  * @param  pwr_ptr: структура управления питанием
  * @param  suart_ptr: устройство I2C для общения с монитором питания
	* @retval статус успешности инициализации: кол-во успешно инициализированных блоков
  */
int8_t pwr_init(type_PWR_CONTROL* pwr_ptr, type_SysUART* suart_ptr)
{
	int8_t report = 0;
	uint8_t num;
	float max_5V0[PWR_CH_NUMBER] = PWR_5V0_MAX;
	float min_5V0[PWR_CH_NUMBER] = PWR_5V0_MIN;
	float max_3V3[PWR_CH_NUMBER] = PWR_3V3_MAX;
	float min_3V3[PWR_CH_NUMBER] = PWR_3V3_MIN;
	float p_max[PWR_CH_NUMBER] = PWR_MAX;
	float p_min[PWR_CH_NUMBER] = PWR_MIN;
	uint8_t pwr_ch_i2c_addr[PWR_CH_NUMBER] = PWR_I2C_ADDR;
	//
	pwr_ptr->suart_ptr = suart_ptr;
	//
	pwr_ptr->last_call_time_us = 0;
	pwr_ptr->call_interval_us = 0;
	//установка управляющих переменных
	pwr_ptr->ch_read_queue = 0;
	pwr_ptr->ch_queue_mode = 0;
	pwr_ptr->ch_state = PWR_STATE_DEFAULT;
	pwr_ptr->ch_status = 0x0000;
	pwr_ptr->ch_presence = 0x0000;
	// установка парамеров отдельных каналов
	for(num=0; num<PWR_CH_NUMBER; num++){
		report += pwr_ch_init(&pwr_ptr->ch[num], suart_ptr, pwr_ch_i2c_addr[num]);
		pwr_ch_set_bound(&pwr_ptr->ch[num], max_5V0[num], min_5V0[num], 
											max_3V3[num], min_3V3[num], 
											p_max[num], p_min[num]);
	}
	// контроль инициализации
	pwr_ptr->init_report = report;
	printf("\tPwr subsystem init: %d\n", pwr_ptr->init_report);
	//
	return report;
}

/**
  * @brief  организация работы во временнЫх слотах
  * @param  pwr_ptr: структура управления питанием
	* @param  period_ms: период вызова функции
  */
void pwr_process(type_PWR_CONTROL* pwr_ptr, uint32_t period_ms)
{
	uint8_t mode, error;
	if ((pwr_ptr->ch_queue_mode & 0x01) == 1){
		pwr_ptr->ch_read_queue += 1;
		pwr_ptr->ch_queue_mode = 0;
	}
	else{
		pwr_ptr->ch_queue_mode = 1;
	}
	//
	if (pwr_ptr->ch_read_queue >= PWR_CH_NUMBER){
		//запускаем новые измерения
		pwr_ptr->ch_read_queue = 0;
	}
	// вызов времязависимых обсчётчиков для каналов
	for(uint8_t ch_num = PWR_CH_IM; ch_num < PWR_CH_NUMBER; ch_num++) {
		pwr_ch_process(&pwr_ptr->ch[ch_num], period_ms);
	}
	//
	if (pwr_ptr->ch_read_queue == PWR_CH_IM){ // интерфейсный модуль не имеет данных о своем питании
		pwr_ptr->ch_presence |= ((1<<(pwr_ptr->ch_read_queue)) & 0xFFFF);
	}
	else{
		switch(pwr_ptr->ch_queue_mode){
			case(0):
				pwr_ch_get_data(&pwr_ptr->ch[pwr_ptr->ch_read_queue]);
				pwr_ptr->u_5v_pwr_fp[pwr_ptr->ch_read_queue] = pwr_ptr->ch[pwr_ptr->ch_read_queue].u_5v_pwr;
				pwr_ptr->i_5v_pwr_fp[pwr_ptr->ch_read_queue] = pwr_ptr->ch[pwr_ptr->ch_read_queue].i_5v_pwr;
				pwr_ptr->u_5v_self_fp[pwr_ptr->ch_read_queue] = pwr_ptr->ch[pwr_ptr->ch_read_queue].u_5v_self;
				pwr_ptr->u_3v3_self_fp[pwr_ptr->ch_read_queue] = pwr_ptr->ch[pwr_ptr->ch_read_queue].u_3v3_self;
			break;
			case(1):
				if (pwr_ch_check_rd_data(&pwr_ptr->ch[pwr_ptr->ch_read_queue]) == 0){
					pwr_ptr->ch_presence = pwr_ptr->ch_presence & (~(1<<(pwr_ptr->ch_read_queue)) & 0xFFFF);
				}
				else{
					pwr_ptr->ch_presence = pwr_ptr->ch_presence  | ((1<<(pwr_ptr->ch_read_queue)) & 0xFFFF);
					//установка состояния канала
					mode = (pwr_ptr->ch_state >> pwr_ptr->ch_read_queue) & 0x01;
					pwr_ch_on_off(&pwr_ptr->ch[pwr_ptr->ch_read_queue], mode);
					//
					pwr_ch_get_error(&pwr_ptr->ch[pwr_ptr->ch_read_queue], &error);
					if (error) {
						pwr_ptr->ch_status |= (1 << pwr_ptr->ch_read_queue);
					}
				}
			break;
		}
	}
}

void pwr_state_reset(type_PWR_CONTROL* pwr_ptr)
{
	pwr_ptr->ch_state = PWR_STATE_DEFAULT;
	pwr_ptr->ch_status = 0x00;
	pwr_ptr->ch_presence = 0x00;
}

void pwr_get_tmi_report(type_PWR_CONTROL* pwr_ptr, type_PWR_TMI_REPORT* tmi_report_ptr)
{
	uint8_t i=0;
	tmi_report_ptr->status = pwr_ptr->ch_status;
	tmi_report_ptr->state = pwr_ptr->ch_state;
	tmi_report_ptr->presence = pwr_ptr->ch_presence;
	for (i=0; i<PWR_CH_NUMBER; i++) {
		tmi_report_ptr->voltage[i] = (uint16_t)(pwr_ptr->ch[i].u_5v_self * 256);
		tmi_report_ptr->current[i] = (uint16_t)(pwr_ptr->ch[i].i_5v_pwr * 256);
	}
}

/**
	* @brief  функция для запуска в планировщике задач обработки каналов питания
	* @param  ctrl_struct указатель на программную модель устройства
	* @param  time_ms глобальное время
  */
int8_t pwr_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	type_PWR_CONTROL* pwr_ptr = (type_PWR_CONTROL*)ctrl_struct;
	if ((time_us - (pwr_ptr->last_call_time_us)) > (uint64_t)(PWR_PROCESS_PERIOD*1000)) {
		pwr_ptr->call_interval_us = time_us - pwr_ptr->last_call_time_us;
		pwr_ptr->last_call_time_us = time_us;
		//
		pwr_process(pwr_ptr, pwr_ptr->call_interval_us / 1000);
		//
		return 1;
	}
	else {
		return 0;
	}
}
