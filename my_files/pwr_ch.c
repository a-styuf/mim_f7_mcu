/**
  ******************************************************************************
  * @file           : pwr_ch.c
  * @version        : v1.0
  * @brief          : надстройка над CubeMX для удобного управления отдельным каналом питания
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "pwr_ch.h"

/**
  * @brief  установка границ допустимых параметров питания
  * @param  pwr_ch_ptr: структура управления каналом питания
  * @param  suart_ptr: канал чтения/установки параметров питания
  * @param  addr: I2C адрес
	* @retval 1 - успешная инициализация, 0 - ошибка
  */
int8_t pwr_ch_init(type_PWR_CHANNEL* pwr_ch_ptr, type_SysUART* suart_ptr, uint8_t addr)
{
	float b[6] = PWR_CH_BOUNDS_DEFAULT;

	pwr_ch_ptr->suart_ptr = suart_ptr;
	pwr_ch_ptr->i2c_addr = addr;
	pwr_ch_ptr->mode = PWR_CH_OFF;

	pwr_ch_ptr->u_5v_pwr = 0.; 
	pwr_ch_ptr->i_5v_pwr = 0.;
	pwr_ch_ptr->u_5v_self = 0.;
	pwr_ch_ptr->u_3v3_self = 0.;

	pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_NO_ERR, PWR_CH_STATUS_SET);
	pwr_ch_ptr->error_old = PWR_CH_ERR_NO_ERR;
	// выставляем начальные значения проверок ошибок на таймаут
	pwr_ch_ptr->error_check_inh = 1;
	pwr_ch_ptr->error_check_timeout_ms = 0;
	pwr_ch_set_bound(pwr_ch_ptr, b[0], b[1], b[2], b[3], b[4], b[5]);
	return 1;
}

/**
 * @brief функция обработки состояний требующих времени
 * 
 * @param pwr_ch_ptr 
 * @param period_ms период вызова
 */
void pwr_ch_process(type_PWR_CHANNEL* pwr_ch_ptr, uint32_t period_ms)
{
	// проверки таймаутов на проверки
	if (pwr_ch_ptr->error_check_timeout_ms < PWR_CH_ERROR_CHECK_TIMEOUT_MS){
		pwr_ch_ptr->error_check_timeout_ms += period_ms;
		pwr_ch_ptr->error_check_inh = 1;
	}
	else{
		pwr_ch_ptr->error_check_inh = 0;
	}
}

/**
  * @brief  установка границ допустимых параметров питания
  * @param  pwr_ch_ptr: структура управления каналом питания
  * @param  max_5v0: верхняя граница питания
  * @param  min_5v0: нижняя граница напряжения граница питания
  * @param  max_3v3: верхняя граница питания
  * @param  min_3v3: нижняя граница напряжения граница питания
  * @param  pow_max: верхняя граница мощности
  * @param  pow_min: нижняя граница мощности
  */
void pwr_ch_set_bound(type_PWR_CHANNEL* pwr_ch_ptr, float max_5v0, float min_5v0, float max_3v3, float min_3v3, float pow_max, float pow_min)
{
	pwr_ch_ptr->power_max = pow_max;
	pwr_ch_ptr->power_min = pow_min;
	pwr_ch_ptr->max_5v0 = max_5v0;
	pwr_ch_ptr->min_5v0 = min_5v0;
	pwr_ch_ptr->max_3v3 = max_3v3;
	pwr_ch_ptr->min_3v3 = min_3v3;
}

/**
  * @brief  проверка параметров питания
  * @param  pwr_ch_ptr: структура управления каналом питания
  * @param  error: тип ошибки питания
  * @retval 1 - изменилось состояние питания, 0 - ошибки остались, какие и были
  */
uint8_t pwr_ch_get_error(type_PWR_CHANNEL* pwr_ch_ptr, uint8_t *error)
{
	uint8_t retval = 0;
	float voltage = 0, v_3v3 = 0, current = 0, power = 0;
	pwr_ch_ptr->error_old = pwr_ch_ptr->error;
	//
	voltage = pwr_ch_ptr->u_5v_self;
	v_3v3 = pwr_ch_ptr->u_3v3_self;
	current = pwr_ch_ptr->i_5v_pwr;
	//
	power = voltage*current;
	//
	if(pwr_ch_ptr->error_check_inh == 0) {
		if (pwr_ch_ptr->mode != pwr_ch_ptr->mode_rd){
			pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_SWITCH, PWR_CH_STATUS_SET);
		}
		if (pwr_ch_ptr->mode == PWR_CH_OFF){
			if (voltage > pwr_ch_ptr->min_5v0){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_SWITCH, PWR_CH_STATUS_SET);
			}
			else if (power > pwr_ch_ptr->power_min){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_SWITCH, PWR_CH_STATUS_SET);
			}
		}
		else if(pwr_ch_ptr->mode == PWR_CH_ON){
			if ((voltage <= pwr_ch_ptr->min_5v0) || (voltage > pwr_ch_ptr->max_5v0) ){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_VOLTAGE, PWR_CH_STATUS_SET);
			}
			else if ((v_3v3 <= pwr_ch_ptr->min_3v3) || (v_3v3 > pwr_ch_ptr->max_3v3) ){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_VOLTAGE, PWR_CH_STATUS_SET);
			}
			else if (power > pwr_ch_ptr->power_max){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_PWR, PWR_CH_STATUS_SET);
			}
			else if (power <= pwr_ch_ptr->power_min){
				pwr_ch_error_collector(pwr_ch_ptr, PWR_CH_ERR_SWITCH, PWR_CH_STATUS_SET);
			}
		}
	}
	//для определения изменения значения с нуля на 1 используется для old и new: (old^new)&new
	if ((((pwr_ch_ptr->error ^ pwr_ch_ptr->error_old) & pwr_ch_ptr->error_old)) & PWR_CH_STATUS_MASK){
		retval = 1;
	}
	*error = pwr_ch_ptr->error;
	return retval;
}

/**
  * @brief  включение/отключение питания выбранной ПН или канала ПН
  * @param  pwr_ch_ptr: структура управления каналом питания
  * @param  mode: 1 - включить, 0 -отключить, обрезается по (&0x01)
  */
void pwr_ch_on_off(type_PWR_CHANNEL* pwr_ch_ptr, uint8_t mode) //работает для всех каналов, кроме 0-го (МС) - он всегда включен
{
	uint8_t data[1];
	if((pwr_ch_ptr->mode & 0x01) != (mode & 0x01)){
		// запрещаем на какое-то время проверки параметров питания
		pwr_ch_ptr->error_check_timeout_ms = 0;
		pwr_ch_ptr->error_check_inh = 1;
		//
		if (mode&0x01){
			data[0] = 0x01;
			suart_write_i2c(pwr_ch_ptr->suart_ptr, pwr_ch_ptr->i2c_addr, 9, data, 1);
			pwr_ch_ptr->mode = PWR_CH_ON;
		}
		else{
			data[0] = 0x00;
			suart_write_i2c(pwr_ch_ptr->suart_ptr, pwr_ch_ptr->i2c_addr, 9, data, 1);
			pwr_ch_ptr->mode = PWR_CH_OFF;
		}
	}
}

/**
  * @brief  запрос данных канала
  * @param  pwr_ch_ptr: структура управления каналом питания
  */
void pwr_ch_get_data(type_PWR_CHANNEL* pwr_ch_ptr)
{
	suart_read_i2c(pwr_ch_ptr->suart_ptr, pwr_ch_ptr->i2c_addr, 0, NULL, 12);
}

/**
  * @brief  запрос данных канала
  * @param  pwr_ch_ptr: структура управления каналом питания
  */
int8_t pwr_ch_check_rd_data(type_PWR_CHANNEL* pwr_ch_ptr)
{
	uint8_t i2c_data[128] = {0}, i2c_len = 0;
	type_MIM_PWR_STRUCT pwr_info;
	if(pwr_ch_ptr->suart_ptr->rx_ready){
		if(suart_get_i2c_last_data(pwr_ch_ptr->suart_ptr, i2c_data, &i2c_len)){
			memcpy((uint8_t*)&pwr_info, i2c_data, sizeof(type_MIM_PWR_STRUCT));
			pwr_ch_ptr->pwr_info = pwr_info;
			pwr_ch_update_pwr_data(pwr_ch_ptr);
			return 1;
		}
		else{
			return 0;
		}
	}
	else{
		return 0;
	}
}

/**
 * @brief пересчет данных из сырой структуры с параметрами питания в реальные значения
 * 
 * @param pwr_ch_ptr 
 */
void pwr_ch_update_pwr_data(type_PWR_CHANNEL* pwr_ch_ptr)
{
	pwr_ch_ptr->u_5v_pwr = pwr_ch_ptr->pwr_info.UPwr5V / 1000.; 
	pwr_ch_ptr->i_5v_pwr = (pwr_ch_ptr->pwr_info.IPwr5V / 1000.)/(PWR_CH_I_R_SHUNT_OHM*PWR_CH_I_ADM4073_K);
	pwr_ch_ptr->u_5v_self = pwr_ch_ptr->pwr_info.U5V / 1000.;
	pwr_ch_ptr->u_3v3_self = pwr_ch_ptr->pwr_info.U3V3 / 1000.;
	pwr_ch_ptr->mode_rd = (pwr_ch_ptr->pwr_info.PwrOn & 0x01) ? PWR_CH_ON : PWR_CH_OFF;
}

/**
 * @brief обработка статусов канала питания
 * для очистки статуса необходимо использовать флаг NO_ERR с типом SET, при этом сбрасывается счетчик ошибок
 * при записи ошибки инкриминируется счетчик ошибок
 * @param pl_kkd_ptr 
 * @param flag флаг статуса (битовая маска)
 * @param type установка или очистка
 */
void pwr_ch_error_collector(type_PWR_CHANNEL* pwr_ch_ptr, uint32_t flag, uint8_t type)
{
	uint32_t status_mask = PWR_CH_STATUS_MASK;
	//
	switch (flag){
		case (PWR_CH_ERR_NO_ERR):
			pwr_ch_ptr->error = 0x00 & status_mask;
			pwr_ch_ptr->error_cnt = 0x00;
		break;
		default:
			switch(type){
				case PWR_CH_STATUS_SET:
					pwr_ch_ptr->error |= (flag & status_mask);
					pwr_ch_ptr->error_cnt++;
					break;
				case PWR_CH_STATUS_RELEASE:
					pwr_ch_ptr->error &= ~(flag & status_mask);
					break;
				default:
					break;
			}
		break;
	}
}
