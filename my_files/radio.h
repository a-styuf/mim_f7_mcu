#ifndef _RADIO_H_
#define _RADIO_H_

#include "debug.h"
#include "task_planner.h"
#include "flight_task.h"
#include "my_gpio.h"
#include "pwr.h"
#include "frames.h"
#include "clock.h"
#include "canvar.h"
#include "segmented_data.h"
#include "ivar_data.h"

#define RADIO_PROCESS_PERIOD_MS 5000

#define RADIO_TX_FIFO_DEPTH 2
#define RADIO_RX_FIFO_DEPTH 4
#define RADIO_MSG_MAX_LEN 512

// CAN OFFSETS
#define RADIO_CAN_OFFS_STATUS 	   	0
#define RADIO_CAN_OFFS_DATA		    16

// RADIO status and error flags
#define RADIO_STATUS_NO_ONE 				(0)
#define RADIO_STATUS_WORK	 				(1<<0)
#define RADIO_STATUS_CAN_ERR 				(1<<1)
#define RADIO_STATUS_RX_FIFO_NOT_EMPTY 		(1<<2)
#define RADIO_STATUS_TX_FIFO_NOT_EMPTY		(1<<3)
//
#define RADIO_PWR_ERR_SHIFT			4
#define RADIO_PWR_ERR_MASK			(0xF << RADIO_PWR_ERR_SHIFT) //длина 4 бита
//
#define RADIO_STATUS_MODULE_0 		(1<<8)
#define RADIO_STATUS_MODULE_1 		(1<<9)
#define RADIO_STATUS_MODULE_2 		(1<<10)
#define RADIO_STATUS_MODULE_3 		(1<<11)
#define RADIO_STATUS_RX_FIFO_FULL 	(1<<12)
#define RADIO_STATUS_TX_FIFO_FULL	(1<<13)
#define RADIO_STATUS_TX_MODULE		(1<<14)
#define RADIO_OTHER_ERROR 			(1<<15)

#define RADIO_STATUS_SET				(1)
#define RADIO_STATUS_RELEASE			(0)

// события планировщика задач
#define RADIO_TP_EVENT_UPDATE_TMI			(1<<0)
#define RADIO_TP_EVENT_UPDATE_REPORT		(1<<1)
#define RADIO_TP_EVENT_UPDATE_LAST_FRAME	(1<<2)
#define RADIO_TP_EVENT_UPDATE_STATUS		(1<<3)

// управление интервалами
enum radio_interval_list
{
	RADIO_POLLING_INTERVAL, RADIO_TX_INTERVAL, RADIO_RX_INTERVAL,
	RADIO_INTERV_NUMBER
};

#define DEFAULT_RADIO_MIN_S							{0.1, 		0.1, 	0.1}
#define DEFAULT_RADIO_MAX_S							{3600.0,	3600.0, 3600.0}

#define DEFAULT_RADIO_INTERV_VALUES_S				{5.0, 1.0, 0.6}
#define DEFAULT_RADIO_DEFAULT_START_TIME_S			{3.0, 4.0, 5.0}

#define RADIO_SOM_NUM	(4)

#pragma pack(push, 2)

/** 
  * @brief  структура отчета RADIO
  */
typedef union {
	struct{
		uint8_t 	self_id;      		//+0
		uint8_t 	reserve;	      	//+1
		uint16_t 	status;   			//+2
		uint16_t 	error_cnt; 			//+4
		uint16_t 	u_5v0;      		//+6
		uint16_t 	u_3V3;      		//+8
		uint16_t 	current;      		//+10
		uint16_t 	rx_cnt;      		//+12
		uint16_t 	tx_cnt;      		//+14
	}field;
	uint16_t array[8];
}type_RADIO_Report; 			// 16

typedef union {
	struct{
		uint8_t cfg[16];						// +0
		uint16_t status;						// +16
		uint8_t pl_report[16];					// +18
		uint8_t reserve[94];					// +34
		typePktRx packet;  						// +128
		uint8_t reserve_1[84];					// +428
	}field;
	uint8_t array[512];
}type_RADIO_TP_SHARED_MEM; 				// 512


typedef struct{
	typeIVarTxSet array[RADIO_TX_FIFO_DEPTH];
	uint8_t rec_num, rec_max;
} type_RADIO_TX_Fifo;

/**
 * @brief структура управления передатчиком
 * 
 */
typedef struct{
	// копии регистров из модулей
	typeIVarTxSet IVarTxSet_rd;
	typeIVarTxCtrl IVarTxCtrl_rd;
	// данные для записи
	typeIVarTxSet IVarTxSet_wr;
	typeIVarTxCtrl IVarTxCtrl_wr;
	//
	type_RADIO_TX_Fifo fifo;
	uint8_t fifo_ena;
	//
	uint8_t test_frame_num;
	//
	uint32_t frame_cnter;
}type_RADIO_TX_CTRL;


typedef struct{
	typePktRx array[RADIO_RX_FIFO_DEPTH];
	uint8_t rec_num, rec_max;
} type_RADIO_RX_Fifo;

/**
 * @brief структура управления приемником
 * 
 */
typedef struct{
	// параметры для чтения
	typePktRx  IVarPktR_rd; 			
	typeIVarRxPktInfo IVarRxPktInfo_rd;
	// копии регистров из модулей
	typeIVarRxSet IVarRxSet_rd[4];
	typeIVarRxCtrl IVarRxCtrl_rd;
	// данные для записи в регистры
	typeIVarRxSet IVarRxSet_wr[4];
	typeIVarRxCtrl IVarRxCtrl_wr;
	// копии регистров из модулей
	//
	type_RADIO_RX_Fifo fifo;
	uint8_t fifo_ena;
	//
	uint8_t need_to_polling;
	uint32_t frame_cnter;
}type_RADIO_RX_CTRL;

/** 
  * @brief  структура программной модели ПН ККД
  */
typedef struct {
	// уникальные настройки
	uint16_t self_id;
	uint16_t can_addr;
	// внешние интерфейсы
	type_PWR_CHANNEL* pwr_ch_ptr;
	// статусы и ошибки
	uint16_t status; // также содержит флаги ошибок питания
	uint16_t error_cnter;
	//для работы с переменными CAN
	uint8_t can_status[8];
	// интервалы работы
	//! работа с различными интервалами
	uint16_t intervals[RADIO_INTERV_NUMBER];
	uint64_t last_call_interval_times[RADIO_INTERV_NUMBER];
	uint64_t first_call_time[RADIO_INTERV_NUMBER];
	uint8_t first_call_status[RADIO_INTERV_NUMBER];
	// данные RADIO
	type_RADIO_Report report;
	// трансмиттер
	type_RADIO_TX_CTRL tx_ctrl;
	// ресивер
	type_RADIO_RX_CTRL rx_ctrl;
	// внутренние флаги событий
	uint8_t need_to_update_report;
	uint8_t need_to_update_rx_packet;
	//! переменные для работы с планировщиком задач
	uint64_t last_call_time_us, call_interval_us;
}type_RADIO;


#pragma pack(pop)

void radio_init(type_RADIO* radio_ptr, uint16_t self_id, type_PWR_CHANNEL* pwr_ch_ptr, uint16_t can_addr);
//
int8_t radio_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
//
void radio_create_report(type_RADIO* radio_ptr);

// функции управления RADIO
int8_t radio_check_status(type_RADIO* radio_ptr);
void radio_get_packet(type_RADIO* radio_ptr);
void radio_get_settings(type_RADIO* radio_ptr);
void radio_set_settings(type_RADIO* radio_ptr);
//
void radio_intervals_init(type_RADIO* mim_ptr);
int8_t radio_interval_processor(type_RADIO* radio_ptr, uint8_t interval_id, uint64_t time_us);
void radio_set_interval_value(type_RADIO* radio_ptr, uint16_t interval_number, uint16_t interval_value_s);
//
void radio_form_tx_cfg(type_RADIO* radio_ptr, uint8_t bw, uint8_t sf, uint8_t cr, uint8_t crc_, uint8_t ldr, uint8_t sync, uint32_t r_freq, uint8_t f_pwr);
//
void radio_form_tx_cfg(type_RADIO* radio_ptr, uint8_t bw, uint8_t sf, uint8_t cr, uint8_t crc_, uint8_t ldr, uint8_t sync, uint32_t r_freq, uint8_t f_pwr);
void radio_form_tx_data(type_RADIO* radio_ptr, uint8_t* data, uint8_t len);
void radio_tx_data(type_RADIO* radio_ptr, uint8_t mode);
void radio_tx_send_cmd(type_RADIO* radio_ptr, uint8_t cmd);
void radio_tx_test_data(type_RADIO* radio_ptr);
int8_t radio_tx_get_settings(type_RADIO* radio_ptr);
int8_t radio_tx_get_cmd_status(type_RADIO* radio_ptr);
int8_t radio_write_tx_fifo(type_RADIO* radio_ptr, typeIVarTxSet* tx_frame_ptr);
int8_t radio_read_tx_fifo(type_RADIO* radio_ptr, typeIVarTxSet* tx_frame_ptr);
//
int8_t radio_rx_get_rx_info(type_RADIO* radio_ptr);
int8_t radio_rx_check_info(type_RADIO* radio_ptr);
int8_t radio_rx_get_packet(type_RADIO* radio_ptr);
int8_t radio_rx_set_settings(type_RADIO* radio_ptr, typeIVarRxSet* rx_settings);
int8_t radio_rx_send_cmd(type_RADIO* radio_ptr, uint8_t cmd);
int8_t radio_rx_get_cmd_status(type_RADIO* radio_ptr);
int8_t radio_rx_get_settings(type_RADIO* radio_ptr);
int8_t radio_write_rx_fifo(type_RADIO* radio_ptr, typePktRx* rx_frame_ptr);
int8_t radio_read_rx_fifo(type_RADIO* radio_ptr, typePktRx* rx_frame_ptr);
//
void  radio_status_collector(type_RADIO* radio_ptr, uint16_t flag, uint8_t type);
void  radio_status_reset(type_RADIO* radio_ptr);
//
uint16_t radio_get_val_from_bound(uint16_t val, uint16_t min, uint16_t max);
//
uint32_t generate_can_id_mask(uint32_t dev_id, uint32_t var_id, uint32_t offset, uint32_t rtr);

#endif
