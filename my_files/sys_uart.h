#ifndef _SYS_UART_H
#define _SYS_UART_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "debug.h"

#define SUART_MAX_PACKET_LEN 	(256)

#define SUART_STATUS_TIMEOUT	(1<<0)

#define SUART_CMD_I2C_WRITE     (1)
#define SUART_CMD_I2C_READ      (2)

#pragma pack(push, 1)

typedef struct {
	uint8_t  dev_addr;
	uint8_t  res1;
	uint16_t addr;   //little-endian !
	uint8_t count;
	uint8_t data[128];
}typeI2CRWReq;


typedef union{
	struct{
		uint8_t leng;
		uint8_t cmd;   // запрос : (cmd),  ответ :  (cmd | 0x80)
		int8_t status;  // -1 - ошибка
		union{
			uint8_t data[SUART_MAX_PACKET_LEN-3];
			typeI2CRWReq i2c;
		} prms;
	} fields;
	uint8_t array[SUART_MAX_PACKET_LEN];
} typeFrame;

typedef struct
{
	UART_HandleTypeDef *huart_ptr;
	TIM_HandleTypeDef *tim_ptr;
	typeI2CRWReq i2c_rx_data, i2c_tx_data;
	//
	typeFrame rx_frame;
	uint8_t rx_frame_expected_len;
	uint8_t rx_buffer[SUART_MAX_PACKET_LEN];
	uint8_t rx_len;
	uint8_t rx_ready;
	//
	typeFrame tx_frame;
	uint8_t tx_buffer[SUART_MAX_PACKET_LEN];
	uint8_t tx_len;
	//
	uint8_t rx_wait_flag;
	//
	uint16_t status;
	uint16_t error_cnt;
} type_SysUART;

#pragma pack(pop)

int8_t suart_init(type_SysUART* suart_ptr, UART_HandleTypeDef* huart_ptr, TIM_HandleTypeDef* tim_ptr);
void suart_send_cmd(type_SysUART* suart_ptr, uint8_t cmd, int8_t status, uint8_t* data, uint8_t len);
void suart_write_i2c(type_SysUART* suart_ptr, uint8_t dev_addr, uint8_t addr, uint8_t* data, uint8_t len);
void suart_read_i2c(type_SysUART* suart_ptr, uint8_t dev_addr, uint8_t addr, uint8_t* data, uint8_t len);
void suart_rx_irq_handler(type_SysUART* suart_ptr);
void suart_timer_irq_handler(type_SysUART* suart_ptr);
int8_t suart_rx_get_data(type_SysUART* suart_ptr, uint8_t* data, uint8_t* len);
int8_t suart_get_i2c_last_data(type_SysUART* suart_ptr, uint8_t* i2c_data, uint8_t* i2c_len_ptr);
int8_t suart_check_i2c_rx_frame(type_SysUART* suart_ptr);

#endif //_SYS_UART_H
