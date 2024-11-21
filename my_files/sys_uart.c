/**
  ******************************************************************************
  * @file           : pwr_ch.c
  * @version        : v1.0
  * @brief          : надстройка над CubeMX для удобного управления отдельным каналом питания
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */
	
#include "sys_uart.h"

/**
 * @brief 
 * 
 * @param suart_ptr 
 * @param huart_ptr 
 * @param tim_ptr 
 * @return int8_t 
 */
int8_t suart_init(type_SysUART* suart_ptr, UART_HandleTypeDef* huart_ptr, TIM_HandleTypeDef* tim_ptr)
{
	suart_ptr->huart_ptr = huart_ptr;
	//
	suart_ptr->tim_ptr = tim_ptr;
	suart_ptr->tim_ptr->Instance->SR = 0;
	//
	memset((uint8_t*)&suart_ptr->rx_frame, 0x00, sizeof(suart_ptr->rx_frame));
	suart_ptr->rx_frame_expected_len = 0;
	memset(suart_ptr->rx_buffer, 0x00, sizeof(suart_ptr->rx_buffer));
	suart_ptr->rx_len = 0;
	suart_ptr->rx_ready = 0;
	//
	memset((uint8_t*)&suart_ptr->tx_frame, 0x00, sizeof(suart_ptr->tx_frame));
	memset(suart_ptr->tx_buffer, 0x00, sizeof(suart_ptr->tx_buffer));
	suart_ptr->tx_len = 0;
	//
	suart_ptr->rx_wait_flag = 0;
	//
	suart_ptr->status = 0;
	suart_ptr->error_cnt = 0;
	//
	return 0;
}

/**
 * @brief 
 * 
 * @param suart_ptr 
 * @param cmd 
 * @param status 
 * @param data 
 * @param len 
 */
void suart_send_cmd(type_SysUART* suart_ptr, uint8_t cmd, int8_t status, uint8_t* data, uint8_t len)
{
	suart_ptr->tx_frame.fields.cmd = cmd;
	suart_ptr->tx_frame.fields.status = status;
	memcpy(suart_ptr->tx_frame.fields.prms.data, data, len);
	suart_ptr->tx_frame.fields.leng = len + 2;
	//
	suart_ptr->tx_len = suart_ptr->tx_frame.fields.leng + 1;
	memcpy(suart_ptr->tx_buffer, suart_ptr->tx_frame.array, suart_ptr->tx_len);
	//
	HAL_UART_Transmit_IT(suart_ptr->huart_ptr, suart_ptr->tx_buffer, suart_ptr->tx_len);
	suart_ptr->rx_wait_flag = 1;
	memset(suart_ptr->rx_buffer, 0x00, sizeof(suart_ptr->rx_buffer));
	memset(suart_ptr->rx_frame.array, 0x00, sizeof(suart_ptr->rx_frame));
	suart_ptr->rx_len = 0;
	suart_ptr->rx_frame_expected_len = 0;
	suart_ptr->rx_ready = 0;
	//
	// HAL_TIM_Base_Stop_IT(suart_ptr->tim_ptr);
	// suart_ptr->tim_ptr->Instance->CNT = 0;
	//
	HAL_TIM_Base_Start_IT(suart_ptr->tim_ptr);
	//
	HAL_UART_Receive_IT(suart_ptr->huart_ptr, suart_ptr->rx_buffer, 1);
}


void suart_write_i2c(type_SysUART* suart_ptr, uint8_t dev_addr, uint8_t addr, uint8_t* data, uint8_t len)
{
	// form i2c data packet
	suart_ptr->i2c_tx_data.dev_addr = dev_addr;
	suart_ptr->i2c_tx_data.res1 = 0xFE;
	suart_ptr->i2c_tx_data.addr = addr;
	suart_ptr->i2c_tx_data.count = len;
	memcpy(suart_ptr->i2c_tx_data.data, data, len);
	// form and send sys_uart data packet
	suart_send_cmd(suart_ptr, SUART_CMD_I2C_WRITE, 0x00, (uint8_t*)&suart_ptr->i2c_tx_data, 5+len);
}

void suart_read_i2c(type_SysUART* suart_ptr, uint8_t dev_addr, uint8_t addr, uint8_t* data, uint8_t len)
{
	// form i2c data packet
	suart_ptr->i2c_tx_data.dev_addr = dev_addr;
	suart_ptr->i2c_tx_data.res1 = 0xFE;
	suart_ptr->i2c_tx_data.addr = addr;
	suart_ptr->i2c_tx_data.count = len;
	memset(suart_ptr->i2c_tx_data.data, 0xFE, sizeof(suart_ptr->i2c_tx_data.data));
	// form and send sys_uart data packet
	suart_send_cmd(suart_ptr, SUART_CMD_I2C_READ, 0x00, (uint8_t*)&suart_ptr->i2c_tx_data, 5);
}

void suart_rx_irq_handler(type_SysUART* suart_ptr)
{
	
	suart_ptr->rx_len++;
	//
	if (suart_ptr->rx_wait_flag){
		if(suart_ptr->rx_len == 1){
			memcpy(&suart_ptr->rx_frame_expected_len, &suart_ptr->rx_buffer[0], 1);
			HAL_UART_Receive_IT(suart_ptr->huart_ptr, suart_ptr->rx_buffer+suart_ptr->rx_len, 1);
		}
		else if (suart_ptr->rx_len <= suart_ptr->rx_frame_expected_len) {
			HAL_UART_Receive_IT(suart_ptr->huart_ptr, suart_ptr->rx_buffer+suart_ptr->rx_len, 1);
		}
		else{
			memcpy(suart_ptr->rx_frame.array, suart_ptr->rx_buffer, suart_ptr->rx_len);
			suart_ptr->rx_ready = 1;
			suart_ptr->rx_wait_flag = 0;
			HAL_TIM_Base_Stop_IT(suart_ptr->tim_ptr);
			suart_ptr->tim_ptr->Instance->CNT = 0;
			HAL_UART_AbortReceive_IT(suart_ptr->huart_ptr);
		}

	}
}

void suart_timer_irq_handler(type_SysUART* suart_ptr)
{
	suart_ptr->rx_ready = 0;
	suart_ptr->rx_wait_flag = 0;
	suart_ptr->rx_frame_expected_len = 0;
	HAL_UART_AbortReceive_IT(suart_ptr->huart_ptr);
	//
	suart_ptr->status |= SUART_STATUS_TIMEOUT;
	suart_ptr->error_cnt += 1;
}

/**
 * @brief получение данных последнего принятого пакета в случае соответствия структуре протокола передачи
 * 
 * @param suart_ptr 
 * @param data указатель на буфер, в которой будет сложена информации
 * @param len длина информации
 * @return int8_t 0 - ошибка, 1 - пакет корректный
 */
int8_t suart_rx_get_data(type_SysUART* suart_ptr, uint8_t* data, uint8_t* len_ptr)
{
	if (suart_ptr->rx_ready == 1){
		suart_ptr->rx_ready = 0;
		if (suart_ptr->rx_frame.fields.status == -1){
			return 0;
		}
		else{
			*len_ptr = suart_ptr->rx_frame.fields.leng;
			memcpy(data, suart_ptr->rx_frame.fields.prms.data, *len_ptr);
			return 1;
		}
	}
	else {
		return 0;
	}
}

/**
 * @brief получение данных последнего обмена I2C в случае соответствия структуре протокола передачи
 * 
 * @param suart_ptr 
 * @param data указатель на буфер, в которой будет сложена полезная информация I2C
 * @param len длина информации
 * @return int8_t 0 - ошибка, 1 - пакет корректный
 */
int8_t suart_get_i2c_last_data(type_SysUART* suart_ptr, uint8_t* i2c_data, uint8_t* i2c_len_ptr)
{
	uint8_t data[256] = {0};
	uint8_t len = 0;
	if (suart_rx_get_data(suart_ptr, data, &len)){
		if ((len == 1) && (data[0] == 0xFF)){
			return 0;
		}
		else{
			if (suart_check_i2c_rx_frame(suart_ptr) > 0){
				memcpy(i2c_data, (uint8_t*)&suart_ptr->rx_frame.fields.prms.i2c.data, suart_ptr->rx_frame.fields.prms.i2c.count);
				return 1;
			}
			else return 0;
		}
	}
	else {
		return 0;
	}
}

/**
 * @brief сравнение отправленного и принятого пакетов i2c на предмет соответствия контрольных пакетов
 * 
 * @param suart_ptr 
 * @return int8_t 	1 - норма, 
 * 					0 - исключение, 
 * 					-1 - ошибка адреса устройства
 * 					-2 - ошибка адреса регистра
 * 					-3 - ошибка длины данных
 * 					-4 - недопустимая длина
 */
int8_t suart_check_i2c_rx_frame(type_SysUART* suart_ptr)
{
	if(suart_ptr->i2c_tx_data.dev_addr != suart_ptr->rx_frame.fields.prms.i2c.dev_addr){
		return -1;
	}
	else if(suart_ptr->i2c_tx_data.addr != suart_ptr->rx_frame.fields.prms.i2c.addr){
		return -2;
	}
	else if(suart_ptr->i2c_tx_data.count != suart_ptr->rx_frame.fields.prms.i2c.count){
		return -3;
	}
	else if((suart_ptr->rx_frame.fields.prms.i2c.count > 0x7F) || (suart_ptr->rx_frame.fields.prms.i2c.count < 5)){
		return -4;
	}
	return 1;
}
