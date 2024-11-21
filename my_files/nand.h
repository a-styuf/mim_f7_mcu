#ifndef _NAND_H
#define _NAND_H

#include "frames.h"
#include "clock.h"
#include "fmc.h"

#define NAND_BANK_ADDR          ((uint32_t)0x80000000)

#ifdef NAND_DEVICE
#undef NAND_DEVICE
#define NAND_DEVICE (NAND_BANK_ADDR)
#endif

#define NAND_FRAME_SIZE         (128)
#define NAND_PAGE_SIZE          (8192)
#define NAND_PAGE_NUM           (265728)

typedef enum{
    NAND_OK, NAND_TIMEOUT, 
}type_NAND_STATE_ENUM;

#pragma pack(push, 2)

/**
 * @brief  Структура управления nand-памятью
 * 
 */
typedef union{
    struct{
		uint8_t 	marker_code;      	//+0
		uint8_t 	device_code;	    //+1
		uint8_t 	int_info;	      	//+2
        uint8_t 	page_info;	      	//+3
        uint8_t 	fifth_code;	      	//+4
        uint8_t 	sixth_code;	      	//+5
	}f;
	uint8_t array[6];
}type_NAND_ID;

/**
 * @brief  Структура управления nand-памятью
 * 
 */
typedef struct{
    NAND_HandleTypeDef *hnand;
    type_NAND_ID id;
    uint8_t wr_frame[NAND_FRAME_SIZE];
    uint8_t rd_frame[NAND_FRAME_SIZE];
    uint8_t page_buffer[NAND_PAGE_SIZE];
    uint32_t line_read_error;
    uint32_t line_write_error;

    uint32_t rd_start_block, rd_current_block;
    uint32_t rd_page_ptr;
    uint32_t rd_frame_ptr, rd_frame_cnt;

    uint32_t wr_start_block, wr_current_block;
    uint32_t wr_page_ptr;
    uint32_t wr_frame_ptr, wr_frame_cnt;
    //
    uint32_t frame_size_b;
    uint32_t page_size_b;
    uint32_t page_size_fr;
    uint32_t block_size_b;
    uint32_t block_size_fr;
    uint32_t block_size_page;
    uint32_t block_num_full;
    uint32_t page_num_full;
    uint32_t frame_num_full;
}type_NAND;

#pragma pack(pop)

void nand_init(type_NAND *nand_ptr);
void nand_reset_parameters(type_NAND *nand_ptr);

int8_t nand_hal_reset_nand(type_NAND *nand_ptr);
int8_t nand_hal_read_id(type_NAND *nand_ptr);

int8_t nand_write_frame(type_NAND *nand_ptr, uint32_t page_num, uint8_t frame_num, uint8_t *frame);
int8_t nand_write_page(type_NAND *nand_ptr, uint32_t page_num, uint8_t *page);
int8_t nand_read_frame(type_NAND *nand_ptr, uint32_t page_num, uint8_t frame_num, uint8_t *frame);
uint32_t nand_hal_read_status(type_NAND *nand_ptr);
int8_t nand_hal_erase(type_NAND *nand_ptr, uint32_t block_num);

int8_t nand_lin_write_block_set(type_NAND *nand_ptr, uint32_t block_num);
int8_t nand_lin_write_frame(type_NAND *nand_ptr, uint8_t *frame);
int8_t nand_lin_write_finish(type_NAND *nand_ptr);

int8_t nand_lin_read_block_set(type_NAND *nand_ptr, uint32_t block_num);
int8_t nand_lin_read_frame(type_NAND *nand_ptr, uint8_t *frame);

#endif
