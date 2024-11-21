#ifndef _EXPERIMENT_H
#define _EXPERIMENT_H

#include <stdlib.h>
#include <stdio.h>
#include "frames.h"
#include "clock.h"
#include "fmc.h"
#include "ivar_data.h"
#include "nand.h"
#include "mim_rs485_tr_lvl_data_map.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define EX_MSG_BUFFER_LENG  	(16)
#define EX_DEFAULT_PERIOD_S		(2)
#define EX_DEFAULT_STEP_NUM		(16)
#define EX_MAX_STEP_NUM			(0xFFFF)

#define EX_FRAME_RX_DATA	 	0x10
#define EX_FRAME_REPORT		 	0x11

#define EX_RX_DATA_FIFO_DEPTH			(32)
#define EX_RX_FRAME_CNTER_TO_SAVE_MAX	(EX_RX_DATA_FIFO_DEPTH/2)

#define EX_STATUS_NO_ONE 				(0)
#define EX_STATUS_WORK	 				(1<<0)
#define EX_STATUS_ERR 					(1<<1)
#define EX_STATUS_TX_EMPTY			 	(1<<2)
#define EX_STATUS_RX_ERROR			 	(1<<3)
#define EX_STATUS_LAUNCH_ERR		 	(1<<4)
#define EX_STATUS_RX_FIFO_FULL		 	(1<<5)
#define EX_STATUS_FRAME_CNTER_EXCEED 	(1<<6)  //  флаг превышения кол-ва кадров для сохранения максимально заданного значения


#define EX_STATUS_SET				(1)
#define EX_STATUS_RELEASE			(0)


typedef enum ex_state{
	EX_IDLE, EX_PREPARATION, EX_TX_WORK, 
	EX_RX_WORK, EX_REPORT, EX_FINISH
}type_ENUM_EX_STATE;

#pragma pack(push, 1)

typedef union{
	struct{
		uint8_t  module_num;					//	+0
		uint8_t  BW;      // 6..9                   +1
		uint8_t  SF;      // 6..12                  +2
		uint8_t  CR;      // 1..4 : 4/5..4/8        +3
		uint8_t  CRC_;    // 0 or 1                 +4
		uint8_t  LDR;     // 0 or 1                 +5
		uint8_t  Sync;    // e.g.  0x12             +6
		uint8_t  InvertIQ;    //                    +7
		uint8_t  Preleng;    //                     +8
		uint32_t RFreq;   // 861000000..1020000000  +12
		uint8_t  FPwr;    // 0..15 : min..max       +13
		uint8_t  PwrNoOn;    //                     +14
		uint8_t  res2[2];    //                     +15
		uint8_t  PLeng;		//						+17
	} f;
	uint8_t array[18];
} type_EX_TX_Param;

typedef struct{
	type_EX_TX_Param msg[EX_MSG_BUFFER_LENG];
	uint8_t buffer_msg_ptr;
} type_RADIO_EX_TX_MSG_BUFFER;

typedef struct{
	uint8_t  dev_id;     	//	+0
  uint8_t  modem_id;   	//	+1
	uint8_t  if_chain;     	//	+2
	uint8_t  reserve[3];   	//	+3
	uint32_t count_ms;		// 	+6
	float rssi_channel;		// 	+10
	float rssi_signal;		// 	+14
	float snr;				// 	+18
	float snr_max;			// 	+22
	float snr_min;			// 	+26
} type_RADIO_EX_RX_DATA;	//  30
#pragma pack(pop)

#pragma pack(push, 2)


typedef struct{
	type_RADIO_EX_RX_DATA array[EX_RX_DATA_FIFO_DEPTH];
	uint8_t rec_num, rec_max;
} type_EX_RX_Data_Fifo;

/** Приемные данные для эксперимента */
typedef union{
	struct{
		type_SingleFrame_Header header; // +0
		//
		uint16_t time_fractional_ms;	// +12
		uint32_t uniq_tx_id;			// +14			
		uint16_t reserve_1;				// +16
		uint8_t tx_parameters[18];		// +20
		uint8_t position_data[16];		// +38
		//
		type_RADIO_EX_RX_DATA rx_data[2];	//+54
		//
		uint8_t reserve_2[12];
		//
		uint16_t crc16; 				//+126
	} fields;
	typeSingleFrameStruct frame;
	uint8_t array[128];
} type_EX_RX_Data;	

/** Отчет по эксперименту */
typedef union{
	struct{
		type_SingleFrame_Header header; // +0
		//
		uint8_t state;				// +12
		uint8_t status;				// +13
		uint16_t error_cnt;			// +14
		uint16_t step_num;			// +16
		uint16_t step_num_max;		// +18
		uint16_t period_s;			// +20
		uint8_t data_len;			// +22
		uint8_t frame_cnter_to_save;// +23
		uint32_t start_nand_block;	// +24
		uint32_t wr_nand_frame_cnter;	// +28
		uint32_t line_read_error;	// +32
    	uint32_t line_write_error;	// +36
    	uint32_t uniq_tx_id;		// +40
    	uint32_t tx_frame_cnter;	// +44
    	uint32_t rx_frame_cnter;	// +48
    	uint32_t rx_saved_frame_cnter;	// +52
		//
		uint8_t reserve[70];		// +56
		//
		uint16_t crc16; 			//+126
	} fields;
	typeSingleFrameStruct frame;
	uint8_t array[128];
} type_EX_REPORT_FRAME;	

typedef struct{
	uint16_t step_num_max;			// +0
	uint16_t period_s;				// +2
	uint32_t start_nand_block;			// +4
	uint8_t data_len;				// +8
	uint8_t frame_cnter_to_save;	// +9
}type_EX_Param;						// 10

/**
  * @brief  управляющие параметры для эксперимента
  */
typedef struct
{ 
	type_NAND *nand_ptr;
	//
	uint8_t self_id;
	uint8_t fabr_num;
	uint8_t dev_id;
	//
	type_ENUM_EX_STATE state;
	uint16_t frame_num;
	uint8_t ex_seq_num;
	uint32_t status;
	uint16_t error_cnt;
	uint32_t tx_frame_cnter;
	uint32_t rx_frame_cnter;
	uint32_t rx_saved_frame_cnter;
	//
	type_EX_Param shadow_param;
	type_EX_Param current_param;
	uint16_t period_s;
	uint8_t data_len;
	uint8_t frame_cnter_to_save;
	uint8_t position_data[16];
	//
	uint16_t step_num, step_num_max;
	uint32_t current_uniq_id;
	//
	uint32_t start_nand_block;
	uint32_t page_num;
	//
	type_RADIO_EX_TX_MSG_BUFFER tx_msg_buffer;
	type_EX_TX_Param last_tx_msg;
	typeIVarTxSet tx_frame;
	//
	type_EX_RX_Data_Fifo rx_data_fifo;
	type_RADIO_EX_RX_DATA last_rx_data;
	uint8_t rd_data_finish_flag;
	//	
	type_EX_RX_Data rx_frame;
	type_EX_REPORT_FRAME report_frame;
	uint8_t rx_nand_buffer[NAND_FRAME_SIZE];
} type_EXPERIMENT_CORE;

#pragma pack(pop)

void ex_init(type_EXPERIMENT_CORE *ex_ptr, type_NAND *nand_ptr, uint8_t self_id, uint8_t fabr_num, uint8_t dev_id);
void ex_reset(type_EXPERIMENT_CORE *ex_ptr);
void ex_set_idle(type_EXPERIMENT_CORE *ex_ptr);
void ex_preparation_step(type_EXPERIMENT_CORE *ex_ptr);
uint8_t ex_tx_work_state_step(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_rx_work_state_step(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_report_step(type_EXPERIMENT_CORE *ex_ptr);

int8_t ex_set_parameters(type_EXPERIMENT_CORE *ex_ptr, uint16_t step_num, uint16_t period_s, uint8_t data_len, uint32_t start_page, uint8_t frame_to_save);
int8_t ex_get_current_parameters(type_EXPERIMENT_CORE *ex_ptr);
type_EX_TX_Param _ex_generate_tx_param(type_EXPERIMENT_CORE *ex_ptr, uint8_t module_num, uint8_t BW, uint8_t SF, uint8_t CR, 
                                                                uint8_t CRC_, uint8_t LDR, uint8_t Sync, uint8_t InvertIQ,
																uint8_t Preleng, uint32_t RFreq, uint8_t FPwr, uint8_t  PwrNoOn,
																uint8_t PLeng);
int8_t ex_set_tx_msg(type_EXPERIMENT_CORE *ex_ptr, uint8_t msg_num, uint8_t *tx_parameters);
int8_t ex_start(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_stop(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_generate_tx_frame(type_EXPERIMENT_CORE *ex_ptr);
uint32_t ex_generate_uniq_id(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_analyze_rx_frame(type_EXPERIMENT_CORE *ex_ptr, type_MIM_INF_ID_RX_DATA rx_frame);
int8_t ex_generate_rx_data_frames(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_generate_report_frames(type_EXPERIMENT_CORE *ex_ptr, uint16_t frame_num);
int8_t ex_generate_report_frames_and_save_to_nand(type_EXPERIMENT_CORE *ex_ptr);
uint32_t ex_generate_uniq_id(type_EXPERIMENT_CORE *ex_ptr);

int8_t ex_rx_fifo_init(type_EXPERIMENT_CORE *ex_ptr);
int8_t ex_rx_fifo_write(type_EXPERIMENT_CORE *ex_ptr, type_RADIO_EX_RX_DATA* rx_data);
int8_t ex_rx_fifo_read(type_EXPERIMENT_CORE *ex_ptr, type_RADIO_EX_RX_DATA* rx_data);

void ex_status_collector(type_EXPERIMENT_CORE *ex_ptr, uint16_t flag, uint8_t type);

#endif
