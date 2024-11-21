#ifndef _SEGMENTED_DATA_H_
#define _SEGMENTED_DATA_H_

#include "debug.h"
#include "crc16.h"

#define SEGM_DATA_MAX_FRAMES 			(19)
#define SEGM_FRAME_DATA_LEN 			(56)
#define SEGM_DATA_MAX_LEN 				(SEGM_DATA_MAX_FRAMES*SEGM_FRAME_DATA_LEN)

#define SEGM_FRAME_LABEL				(0xCDDE)
#define SEGM_FRAME_CRC8_INIT_VAL		(0xCD)

#define SEGM_STATUS_NO_ERROR 			(0x00)
#define SEGM_STATUS_SEGM_LABEL 			(1<<(0))
#define SEGM_STATUS_SEGM_VOL 			(1<<(1))
#define SEGM_STATUS_SEGM_CRC 			(1<<(2))
#define SEGM_STATUS_DATA_LEN 			(1<<(3))

#define SEGM_FRAME_INVALID				(0x00)
#define SEGM_FRAME_VALID				(1<<0)

#pragma pack(push, 2)

/** 
  * @brief  структура отчета RADIO
  */
typedef union {
	struct{
		uint16_t 	label;      				//+0
		uint16_t 	seq_num;      				//+2
		uint8_t 	segm_vol;      				//+4
		uint8_t 	segm_num;      				//+5
		uint8_t 	data_len;      				//+6
		uint8_t 	data[SEGM_FRAME_DATA_LEN];  //+7
		uint8_t 	crc8;      					//+63
	}field;
	uint8_t array[64];
}type_SEGM_Frame; 								// 64

/**
 * @brief 
 */
typedef struct{
	// работа с сегментированными данными
	type_SEGM_Frame last_frame;
	type_SEGM_Frame frame_array[SEGM_DATA_MAX_FRAMES];
	uint8_t frame_status[SEGM_DATA_MAX_FRAMES];
	uint8_t data[SEGM_DATA_MAX_LEN];
	uint16_t data_len;
	uint8_t expected_segm_vol;
	uint32_t expected_seq_num;
	uint8_t data_ready, new_data_ready;
	uint8_t status;
	uint8_t err_cnter;
	uint16_t data_cnter;
	uint16_t frame_cnter;
}type_SEGM;

#pragma pack(pop)

void segm_init(type_SEGM* segm_ptr);
void segm_reset_state(type_SEGM* segm_ptr);
void segm_refresh(type_SEGM* segm_ptr);
void segm_put_new_frame(type_SEGM* segm_ptr, uint8_t* frame);
int8_t segm_check_frame(type_SEGM* segm_ptr, uint8_t* frame);
int8_t segm_check_full_frames(type_SEGM* segm_ptr);
uint8_t segm_get_new_data(type_SEGM* segm_ptr, uint8_t* data);
uint8_t segm_check_new_data(type_SEGM* segm_ptr);
uint8_t segm_get_data(type_SEGM* segm_ptr, uint8_t* data);

#endif
