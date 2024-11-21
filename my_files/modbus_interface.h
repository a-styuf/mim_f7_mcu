#ifndef _MODBUS_INTERFACE_H_
#define _MODBUS_INTERFACE_H_

#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "usart.h"

#define MB_I_TX_DATA_SIZE_MAX (256)
#define MB_I_RX_DATA_SIZE_MAX (256)

#define MB_I_PACKET_TIMEOUT_US (3000)

/** 
  * @brief  структура-интерфейс к UART-периферии
  * @param  init  функция инициализации UART устройства
  * @param  send_packet  функция отправки пакета, en_crc: 1 - подсчет и добавление crc16-ModBus, 0-ничего
  * @param  get_packet  функция приема пакета, возвращает 0 - нет пакета, 1 - есть пакет с правильным crc16, -1 - прием пакета с некорректным crc16
  */
typedef struct
{
  UART_HandleTypeDef* huart;
  
  //uint8_t tx_buffer[MB_I_TX_DATA_SIZE_MAX];
  uint8_t tx_len;
  uint8_t rx_buffer[2 * MB_I_RX_DATA_SIZE_MAX];
  uint8_t rx_ptr;

  uint8_t tx_packet[MB_I_TX_DATA_SIZE_MAX];
  uint8_t tx_ready;
  uint8_t rx_packet[MB_I_RX_DATA_SIZE_MAX];
  uint8_t rx_packet_len;
  uint8_t rx_ready;

  uint32_t packet_num;

  int32_t tx_packet_timeout_us;  //! используем SysTick в прерывания для обновления переменной

} type_MB_Interface;

//
void mb_i_init(type_MB_Interface* mb_i_ptr, UART_HandleTypeDef* huart);
void mb_i_process(type_MB_Interface* mb_i_ptr, uint32_t period_us);
void mb_i_send_packet(type_MB_Interface* mb_i_ptr, uint8_t* buff, uint16_t leng);
int8_t mb_i_get_packet(type_MB_Interface* mb_i_ptr, uint8_t* buff, uint16_t* leng);
int8_t mb_i_packet_in_waiting(type_MB_Interface* mb_i_ptr);
int8_t mb_i_packet_ready(type_MB_Interface* mb_i_ptr);
void mb_i_timer_cb(type_MB_Interface* mb_i_ptr, uint32_t period_us);

void mb_i_it_rx_cb(type_MB_Interface* mb_i_ptr);
void mb_i_it_tx_cb(type_MB_Interface* mb_i_ptr);

#endif
