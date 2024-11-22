#ifndef __MIM_H
#define __MIM_H

#include <stdlib.h>
#include <stdio.h>
#include "my_gpio.h"
#include "rtc.h"
#include "crc16.h"
#include "math.h"
#include "clock.h"
#include "debug.h"
#include "flight_task.h"
#include "task_planner.h"
#include "pwr.h"
#include "mim_rs485_tr_lvl.h"
#include "experiment.h"
#include "radio.h"

#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))

typedef unsigned short uint16_t;

#define MIM_RX_FRAME_FIFO_SIZE			(44)

//! номера используемых подсистем
enum MIM_CONFIG{
	MIM, 
	S0, S1, S2, S3,
	S4, S5, S6, S7,
	ISM0, ISM1, ISM2, ISM3,
	UKV, 
	MIM_MODULE_NUM
};

//! команды в МИМ
enum MIM_CMD{
	MIM_CMD_MODE, MIM_CMD_PWR_STATE, MIM_CMD_SET_DEFAULT, MIM_CMD_RESET_STATE,
	MIM_CMD_NUM
};

//! команды для эксперимента
enum MIM_EXCMD{
	MIM_EXCMD_SETTINGS, MIM_EXCMD_SET_TX_DATA, MIM_EXCMD_GPOSITION, MIM_EXCMD_CLEAR_MSG_BUFFER,
	MIM_EXCMD_NUM
};

//! полетные задания
enum FT_LIST{
	FT_MIM_MAIN, 
	FT_NUM
};

//! режимы работы
enum MODE_LIST{
	MIM_MODE_TRANSCEIVER, MIM_MODE_EXPERIMENT,
	MIM_MODE_NUM
};

//! Типы кадров
#define MIM_FR_TYPE_TMI 			0x00
#define MIM_FR_TYPE_SYS_TMI_PART_1 	0x01
#define MIM_FR_TYPE_SYS_TMI_PART_2 	0x02

// раскрашивание переменных
#define MIM_HANDLER_INTERVAL_MS   			1000

#define MIM_STATUS_TYPE_RESET 				(0)
#define MIM_STATUS_TYPE_SET 				(1)

// биты статуса
#define MIM_STATUS_NO_ONE					(0)
#define MIM_STATUS_OPERATION				(0x01 << 0)
#define MIM_STATUS_S_CH_TRANSFER			(0x01 << 1)
#define MIM_STATUS_EXPERIMENT				(0x01 << 2)
//
#define MIM_STATUS_RX_FIFO_NOT_EMPTY		(0x01 << 8)
#define MIM_STATUS_TX_FIFO_NOT_EMPTY		(0x01 << 9)

//
#define MIM_ERROR_TYPE_RESET 				(0)
#define MIM_ERROR_TYPE_SET 					(1)

// биты ошибок
#define MIM_ERROR_NO_ONE					(0)
#define MIM_ERROR_PWR						(0x01 << 0)
#define MIM_ERROR_RS485_CH1					(0x01 << 1)
#define MIM_ERROR_RS485_CH2					(0x01 << 2)
#define MIM_ERROR_CAN						(0x01 << 3)
#define MIM_ERROR_RX_FIFO					(0x01 << 4)
#define MIM_ERROR_TX_FIFO					(0x01 << 5)
#define MIM_ERROR_CMD_PARAM					(0x01 << 6)
#define MIM_ERROR_EXCMD_PARAM				(0x01 << 7)

// биты статуса сегментированной передачи
#define MIM_SEGM_NO_ONE						(0)
#define MIM_SEGM_SUCCES						(0x01 << 0)
#define MIM_SEGM_ID_ERROR					(0x01 << 1)


#define MIM_SEGM_TYPE_RESET 				(0)
#define MIM_SEGM_TYPE_SET 					(1)


// режим работы


// события планировщика задач
#define MIM_TP_EVENT_UPDATE_TMI				(1 << 0)
#define MIM_TP_EVENT_UPDATE_REPORT			(1 << 1)
#define MIM_TP_EVENT_UPDATE_LAST_FRAME		(1 << 2)
#define MIM_TP_EVENT_UPDATE_STATUS			(1 << 3)
#define MIM_TP_EVENT_TX_EXPERIMENT_DATA		(1 << 4)

enum lm_interval_list
{
	MIM_INTERVAL_1S, MIM_INTERVAL_0_5, MIM_INTERVAL_0_1, MIM_INTERVAL_EXPERIMENT,
	MIM_INTERV_NUMBER
};

#define DEFAULT_MIM_INTERV_VALUES_S					{1, 	0.5, 	0.1, 	1}
#define DEFAULT_MIM_DEFAULT_START_TIME_S			{3, 	3, 		5, 		5}

// описание рабочих структур

#pragma pack(push, 1)

/**
  * @brief  отчет по работе Интерфейсного модуля (16-байт, для помещение в кадр общей телеметрии)
  */
typedef struct
{ 
	uint8_t self_id; 		// +0
	uint8_t reserve; 		// +1
	uint16_t status; 		// +2
	uint16_t err; 			// +4
	uint16_t err_cnt; 		// +6
	uint16_t voltage; 		// +8
	uint16_t current; 		// +10
	uint8_t ver[3]; 		// +12
	//
	uint8_t filler[1]; 		// +15
} type_MIM_REPORT; // 16

/**
  * @brief  короткая телеметрия
  */
typedef union{
	struct{
		uint8_t label;  			// +0
		//
		uint16_t mim_status; 		// +1
		uint16_t err_status; 		// +3
		uint16_t rx_fifo_cnt; 		// +5
		uint32_t rx_total_cnt; 		// +7
		uint32_t tx_total_cnt; 		// +11
		//
		uint8_t crc8; 				//+15
	} fields;
	uint8_t array[16];
} type_MIM_Short_TMI; 			//16

/**
  * @brief  телеметрия
  */
typedef union{
	struct{
		type_SingleFrame_Header header; // +0
		//
		uint16_t mim_status; 		// +12
		uint16_t err_status; 		// +14
		uint16_t error_cnt;			// +16
		//
		uint16_t rx_fifo_cnt; 		// +18
		uint32_t rx_total_cnt; 		// +20
		uint32_t tx_total_cnt; 		// +24
		uint32_t rx_fifo_lost_frames;		// +28
		uint32_t tx_fifo_lost_frames;		// +32
		//
		type_PWR_TMI_REPORT pwr_tmi_report; // +36  //62 байта
		//
		uint8_t segm1_err_cnter;		//+98
		uint8_t segm1_status;	//+99
		uint16_t segm1_data_cnter;	//+100
		uint16_t segm1_trans_cnter;	//+102		
		uint16_t segm1_data_len;	//+104		
		//
		uint8_t segm2_err_cnter;		//+106
		uint8_t segm2_status;	//+107
		uint16_t segm2_data_cnter;	//+108
		uint16_t segm2_trans_cnter;	//+110				
		uint16_t segm2_data_len;	//+112		
		//		
		type_NAND_ID nand_id;	//+114		
		//
		uint8_t reserve[6]; 				//+120
		//
		uint16_t crc16; 					//+126
	} fields;
	typeSingleFrameStruct frame;
	uint8_t array[128];
} type_MIM_TMI;			 			// 128

/**
  * @brief  системная информация
  */
typedef union{
	struct{
		struct{
			type_SingleFrame_Header header; 	// +0
			uint8_t report[7][16]; 				// +12
			uint8_t reserve[2]; 				// +124
			uint16_t crc16; 					// +126
		} p1;
		struct{
			type_SingleFrame_Header header; 	// +0
			uint8_t report[7][16]; 				// +12
			uint8_t reserve[2]; 				// +124
			uint16_t crc16; 					// +126
		} p2;
	} fl;
	typeSingleFrameStruct frame[2];
	uint8_t array[2][128];
} type_MIM_SYS_TMI;			 				// 256

typedef union {
	struct{
		uint8_t array[16];					// +0
		uint16_t status;					// +16
		uint8_t pl_report[16];				// +18
		uint8_t reserve[478];				// +34
	}field;
	uint8_t array[512];
}type_MIM_TP_SHARED_MEM; 		// 512

typedef union {
	struct{
		uint16_t label;				// +0
		uint16_t cmd_num;			// +2
		uint8_t data[12];			// +4
	}fl;
	uint8_t array[16];
}type_MIM_CMD_STRUCT; 		// 16

typedef union {
	struct{
		uint16_t label;				// +0
		uint16_t cmd_num;			// +2
		uint8_t data[28];			// +4
	}fl;
	uint8_t array[32];
}type_MIM_EXP_CMD_STRUCT; 			// 32

typedef struct{
	type_MIM_INF_ID_RX_DATA array[MIM_RX_FRAME_FIFO_SIZE];
	type_MIM_INF_ID_RX_DATA buffer;
	uint8_t wr_ptr, rd_ptr;
	uint8_t rec_num, rec_num_max;
	uint32_t rec_full;
	uint32_t rec_lost;
	uint8_t ena;
} type_MIM_RX_Fifo;

#pragma pack(pop)

#pragma pack(push, 2)

/**
  * @brief  управляющие параметры МС
  */
typedef struct
{ 
	uint32_t global_time_s;
	uint16_t status;
	uint16_t run_block;
	uint16_t err;
	uint16_t err_cnt;
	uint8_t rst_cnt;
	uint8_t load_cfg_status;
	uint16_t mode;
	//! работа с различными интервалами
	uint64_t intervals_us[MIM_INTERV_NUMBER];
	uint64_t last_call_interval_times_us[MIM_INTERV_NUMBER];
	uint64_t first_call_time_us[MIM_INTERV_NUMBER];
	uint8_t first_call_status[MIM_INTERV_NUMBER];
	//
	uint8_t inhibit;  // 0-2 - NU, 3 - запрет проверки оставшегося свободного места для работы циклограмм ИСС
} type_MIM_CTRL;

/** 
  * @brief  структура хранения всех настроек платы
  */
typedef struct
{
	//
	uint8_t self_id;
	uint8_t fabr_num;
	uint8_t dev_id;
	//
	uint16_t ver[3];
	type_MIM_CTRL ctrl;
	type_PWR_CONTROL *pwr_ptr;
	type_RADIO *radio_array;
	//
	type_MIM_TR_LVL_Interface tr_lvl[4];
	//
	type_EXPERIMENT_CORE *ex_ptr;
	//
	type_MIM_REPORT report;
	type_MIM_Short_TMI sh_tmi;
	type_MIM_TMI tmi;
	type_MIM_SYS_TMI sys_tmi;
	uint16_t frame_num;
	//
	uint16_t segm_transaction_cnter;	
	uint16_t segm_cmd_cnter;			
	uint16_t segm_status;				
	uint16_t segm_cmd_data_leng;		
	uint8_t  segm_cmd_id;				
	uint8_t segm_error_cnter;			
	//
	uint8_t need_to_update_report;
	uint8_t need_to_update_status;
	uint16_t pl_status[MIM_MODULE_NUM];
	uint8_t pl_report[MIM_MODULE_NUM][16];
	//
	type_MIM_RX_Fifo rx_fifo;
	//
	uint32_t tx_total_cnt; // заполняется при отправке при разборе разовых команд в main
	uint32_t tx_lost_cnt;
	//! переменные для работы с планировщиком задач
	uint64_t last_call_time_us;
	uint64_t call_interval_us;
} type_MIM_DEVICE;

#pragma pack(pop)

// прототипы функций

//*** ITB ***//
void mim_init(type_MIM_DEVICE* mim_ptr, uint8_t self_id, char* ver, type_PWR_CONTROL* pwr_ptr, uint8_t fabr_num, uint8_t dev_id, type_RADIO *radio_ptr, type_EXPERIMENT_CORE *ex_ptr);
int8_t mim_ctrl_init(type_MIM_DEVICE* mim_ptr);
int8_t mim_state_reset(type_MIM_DEVICE* mim_ptr);

int8_t mim_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
void mim_report_create(type_MIM_DEVICE* mim_ptr);
void mim_reset_state(type_MIM_DEVICE* mim_ptr);

void mim_intervals_init(type_MIM_DEVICE* mim_ptr);
int8_t mim_interval_processor(type_MIM_DEVICE* mim_ptr, uint8_t interval_id, uint64_t time_us);
void mim_set_interval_value(type_MIM_DEVICE* mim_ptr, uint16_t interval_number, uint16_t interval_value_s);

int8_t mim_rx_fifo_init(type_MIM_DEVICE* mim_ptr);
int8_t mim_write_rx_fifo(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr);
int8_t mim_write_rx_fifo_and_save_exp_data(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr);
int8_t mim_read_rx_fifo(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr);
void mim_read_rx_fifo_and_form_mrf_packet(type_MIM_DEVICE* mim_ptr, uint8_t* rx_data);
void mim_rx_fifo_rec_num_update(type_MIM_DEVICE* mim_ptr);

void mim_status_collector(type_MIM_DEVICE* mim_ptr, uint16_t flag, uint8_t type);
void mim_error_collector(type_MIM_DEVICE* mim_ptr, uint16_t flag, uint8_t type);
void mim_segm_status_collector(type_MIM_DEVICE* mim_ptr, uint16_t flag, uint8_t type);

void fill_short_tmi(type_MIM_DEVICE* mim_ptr);
void fill_tmi(type_MIM_DEVICE* mim_ptr);
void fill_sys_tmi(type_MIM_DEVICE* mim_ptr);


uint16_t get_val_from_bound(uint16_t val, uint16_t min, uint16_t max);
uint16_t check_val_in_bound(uint16_t val, uint16_t min, uint16_t max);
uint32_t get_uint32_val_from_bound(uint32_t val, uint32_t min, uint32_t max); //если число внутри границ - используется оно, если нет, то ближайшая граница

#endif
