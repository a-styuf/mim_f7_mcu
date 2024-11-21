#ifndef _MODBUS_H_
#define _MODBUS_H_

#include  <string.h>
#include "modbus_interface.h"  //todo: стандартизовать функции к интерфейсу UART для замены на работу с универсальным UART устройством
#include "task_planner.h"

// дефайны для переменных
#define IB_PROCESS_PERIOD               1
//
#define MB_TRANSACTION_TIMEOUT_MS				10

#define IB_FRAME_TYPE_RX                0x01
#define IB_FRAME_TYPE_TX                0x02
#define IB_FRAME_TYPE_ERROR             0x03

#define IB_GLOBAL_DBG_DEFAULT          (0x00)

#define MB_DEV_ID_NU                    0x0F  //для отправки отладочной информации

#define MB_F_CODE_IDLE									0x00
#define MB_F_CODE_3											0x03
#define MB_F_CODE_6											0x06
#define MB_F_CODE_16										0x10

#define MB_F_CODE_ERROR									0x80

#define MB_FRAME_TYPE_RX      0x01
#define MB_FRAME_TYPE_TX      0x02
#define MB_FRAME_TYPE_ERROR   0x03

#define MB_CRC16_INIT_VAL     0xFFFF

#define MB_UART_RX_STATUS_OK							1
#define MB_UART_RX_STATUS_EXCEPTION				0
#define MB_UART_RX_STATUS_LENGTH					-1
#define MB_UART_CRC_ERROR									-2
#define MB_UART_UNEXPECTED_FRAME					-3

#define MB_TRANSACTION_WRITE_BROAD	0x03
#define MB_TRANSACTION_WRITE				0x02
#define MB_TRANSACTION_READ					0x01
#define MB_TRANSACTION_IDLE 				0x00


#pragma pack(push, 2)

/** 
  * @brief  структура-заготовка для пакетов ModBus
  */
typedef struct{
	uint8_t dev_id;
	uint8_t f_code;
	uint16_t reg_addr;
	uint16_t reg_cnt;
	uint8_t byte_cnt;
	uint16_t data[128];
	uint16_t crc_16;
	uint8_t error_code;
	uint8_t type;
} typeModBusFrameStruct;

/** 
  * @brief  структура управления ModBus
  */
typedef struct
{
  type_MB_Interface trans;
  typeModBusFrameStruct rx_frame, tx_frame;
  uint8_t tx_data[256];
  uint16_t tx_len;
  uint32_t tx_counter;
	uint8_t rx_data[256];
  uint16_t rx_len;
  uint32_t rx_counter;
  uint8_t error_status;
  uint8_t error_counter;
  uint8_t transaction_type;
  //
  uint64_t last_call_time_us;
} typeMBStruct;

#pragma pack(pop)

//
void mb_init(typeMBStruct* mb_ptr, UART_HandleTypeDef* huart);
int8_t mb_process(typeMBStruct* mb_ptr, uint64_t time_us);
void mb_reset_parameters(typeMBStruct* mb_ptr);
int8_t mb_run_transaction(typeMBStruct* mb_ptr, uint8_t dev_id, uint8_t f_code, uint16_t reg_addr, uint16_t reg_cnt, uint16_t* data);
int8_t mb_wait_transaction(typeMBStruct* mb_ptr);
int8_t mb_finish_transaction(typeMBStruct* mb_ptr);
uint8_t mb_get_answer_data(typeMBStruct* mb_ptr, uint8_t* buff, uint16_t len);
uint8_t mb_get_rx_data(typeMBStruct* mb_ptr, uint8_t* buff, uint8_t len);
uint8_t mb_get_tx_data(typeMBStruct* mb_ptr, uint8_t* buff, uint8_t len);
int8_t __mb_parsing_rx_data(typeMBStruct* mb_ptr, uint8_t* data, uint16_t leng, uint8_t transaction_type);
//
void mb_frame_form_packet(typeModBusFrameStruct* frame_ptr, uint8_t* data, uint16_t* leng);
uint16_t mb_frame_calc_crc16(typeModBusFrameStruct* frame_ptr);

//
uint16_t __modbusb_crc16(uint8_t *data, uint16_t len, uint16_t crc16_init);
//

#endif
