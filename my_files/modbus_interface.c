/**
  ******************************************************************************
  * @file           : internal_bus.c
  * @version        : v1.0
  * @brief          : библиотека/объект для управления шиной ModBus
  * @note           : интерфейс к нижнему уровню - UART
  * @author			: Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  * @date			: 2021.09.20
  ******************************************************************************
  */

#include "modbus_interface.h"

/**
	* @brief  инициализация структуры управления внутренней шины
	* @param  mb_i_ptr указатель на структуру управления
  */
void mb_i_init(type_MB_Interface* mb_i_ptr, UART_HandleTypeDef* huart)
{
	memset(mb_i_ptr, 0x00, sizeof(type_MB_Interface));
	mb_i_ptr->huart = huart;
}

/**
 * @brief Функция обсчета таймаута пакета. Так как таймеров ограниченное количество, то можно использовать в каком-нибудь переодическом счетчике, например SysTick.
 * 
 * @param mb_i_ptr 
 * @param period_us 
 */
void mb_i_timer_cb(type_MB_Interface* mb_i_ptr, uint32_t period_us)
{
	if (mb_i_ptr->tx_packet_timeout_us > 0){
		mb_i_ptr->tx_packet_timeout_us -= period_us;
	}
	else{
		mb_i_ptr->tx_packet_timeout_us = 0;
		if ((mb_i_ptr->rx_ptr) && (mb_i_ptr->rx_ready == 0)) {
			mb_i_ptr->rx_packet_len = mb_i_ptr->rx_ptr - mb_i_ptr->tx_len;
			mb_i_ptr->rx_ptr = 0;
			mb_i_ptr->rx_ready = 1;
			mb_i_ptr->packet_num += 1;
			memcpy(mb_i_ptr->rx_packet, mb_i_ptr->rx_buffer + mb_i_ptr->tx_len, mb_i_ptr->rx_packet_len);
		}
	}
}

/**
 * @brief Функция отправки пакета в UART. Не блокирующая.
 * 
 * @param mb_i_ptr указатель на структуру управления
 * @param buff данные на отправку
 * @param leng длина данных на отправку
 */
void mb_i_send_packet(type_MB_Interface* mb_i_ptr, uint8_t* buff, uint16_t leng)
{
	//
	HAL_UART_Transmit_IT(mb_i_ptr->huart, buff, leng);
	memcpy(mb_i_ptr->tx_packet, buff, leng);
	mb_i_ptr->rx_ptr = 0; //обнуляем приемный буфер
	mb_i_ptr->tx_len = leng; //обнуляем приемный буфер
	memset(mb_i_ptr->rx_buffer, 0x00, sizeof(mb_i_ptr->rx_buffer));
	mb_i_ptr->rx_ready = 0;
	mb_i_ptr->tx_ready = 0;
	mb_i_ptr->rx_packet_len = 0;
}

/**
 * @brief Получение последнего принятого пакета
 * 
 * @param mb_i_ptr 
 * @param buff 
 * @param leng 
 * @return int8_t 0 - нет пакета, 1 - пакет получен
 */
int8_t mb_i_get_packet(type_MB_Interface* mb_i_ptr, uint8_t* buff, uint16_t* leng)
{
	if (mb_i_ptr->rx_ready){
		memcpy(buff, mb_i_ptr->rx_packet, mb_i_ptr->rx_packet_len);
		*leng = mb_i_ptr->rx_packet_len;
		mb_i_ptr->rx_ready = 0;
		return 1;
	}
	return 0;
}

int8_t mb_i_packet_in_waiting(type_MB_Interface* mb_i_ptr)
{
	if(mb_i_ptr->tx_packet_timeout_us != 0){
		return 1;
	}
	return 0;
}

int8_t mb_i_packet_ready(type_MB_Interface* mb_i_ptr)
{
	return mb_i_ptr->rx_ready;
}

/**
 * @brief Функция для вызова в прерывание по приему байта через UART
 * 
 * @param mb_i_ptr управляющая структура
 */
void mb_i_it_rx_cb(type_MB_Interface* mb_i_ptr)
{
	mb_i_ptr->rx_ptr++;
	mb_i_ptr->rx_ready = 0;
	HAL_UART_Receive_IT(mb_i_ptr->huart, &mb_i_ptr->rx_buffer[mb_i_ptr->rx_ptr], 1);
	mb_i_ptr->tx_packet_timeout_us = MB_I_PACKET_TIMEOUT_US;
}

/**
 * @brief Функция для вызова в прерывание по отправке пакета байта через UART
 * 
 * @param mb_i_ptr управляющая структура
 */
void mb_i_it_tx_cb(type_MB_Interface* mb_i_ptr)
{
	mb_i_ptr->tx_ready = 1;
	// HAL_UART_Receive_IT(mb_i_ptr->huart, &mb_i_ptr->rx_buffer[mb_i_ptr->rx_ptr], 1);
}
