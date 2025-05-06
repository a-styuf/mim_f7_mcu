#ifndef _EXT_MEM_H
#define _EXT_MEM_H

#include "cy15b104qn_spi.h"
#include "my_gpio.h"
#include "frames.h"
#include "crc16.h"
#include "iwdg.h"

// настройки без учета кадров для сохранения настроек
#define CY15B104_MEM_NUM 2  // кол-во микросхем памяти
#define SINGLE_MEM_VOL_FRAMES (CY15_VOLUME_BYTES/128)  // кол-во кадров в отдельной микросхеме памяти
#define FULL_MEM_VOL_FRAMES (SINGLE_MEM_VOL_FRAMES*CY15B104_MEM_NUM)  // общее количество кадров по 128-байт в физической памяти
// настройки с учетом кадров для сохранения настроек
#define EXT_MEM_SETTINGS_FRAME_NUM 8  // количество кадров для сохранения настроек, последние 2 используются для хранения информации о томах памяти

#define FRAME_MEM_VOL_FRAMES (SINGLE_MEM_VOL_FRAMES*CY15B104_MEM_NUM - EXT_MEM_SETTINGS_FRAME_NUM * CY15B104_MEM_NUM)  // количество кадров в физической памяти за исключением кадров для параметров

#define EXT_MEM_LM_CFG_GENERAL       (0)
#define EXT_MEM_DS2480B_CFG_NUM      (4)
#define EXT_MEM_SELF_CFG_FRAME_1_NUM (EXT_MEM_SETTINGS_FRAME_NUM-1)

#define MEM_DATA_TYPE_SELF_BASE_TMI		(0xE0)
#define MEM_DATA_TYPE_SELF_CFG_1		  (MEM_DATA_TYPE_SELF_BASE_TMI|EXT_MEM_SELF_CFG_FRAME_1_NUM)


/**
  * @brief  задание количества томов
*/
typedef enum EXT_DISK_ENUM
{
  PART_GENERAL, 
  PART_PL1, PART_PL2, PART_PL3, PART_PL4,
  PART_FT0, PART_FT1, PART_FT2, PART_FT3,
  PART_NUM
} type_EXT_MEM_DISK;

#define PART_ALL_MEM 127 // номер-исключение для обращение ко всей памяти целиком

// есть два типа памяти - постоянного размера (например для полетного задания)

// Постоянная память: маппирование в конце физической памяти, по абсолютному размеру
// размер в блоках по 128-байт
#define PART_FT0_VOL             32
#define PART_FT1_VOL             32
#define PART_FT2_VOL             32
#define PART_FT3_VOL             32

#define PART_FULL_CONST          (32*4) // общий размер памяти выделенной под постоянные блоки в 128-байтовых блоков

// Относительная память: маппирование частей памяти распределяемой относительно: сумма частей не должна превышать (полную память - постоянная часть)
// относительный объем памяти выделенной под разные приборы высчитывается как: Vol_1(Fr) = ALL_MEM_VOL_FRAMES * (Vol_1_rel / (PART_FULL_VOL_REL))
#define PART_GENERAL_REL              136
#define PART_PL1_REL                  30
#define PART_PL2_REL                  30
#define PART_PL3_REL                  30
#define PART_PL4_REL                  30

#define PART_FULL_VOL_REL             256

//
#define PART_MODE_READ_BLOCK           0x00  // указатель чтения догоняет указатель записи и блокируется
#define PART_MODE_WRITE_BLOCK          0x01  // указатель записи догоняет указатель чтения и блокируется
#define PART_MODE_REWRITE              0x02  // указатель записи не зависит от указателя чтения
#define PART_MODE_SMART_COIL_WRITE     0x03  // указатель записи толкает указатель чтения в случае достижения оного, указатель чтения блокируется при достижении указателя записи

#define MODE_READ              0x01
#define MODE_WRITE             0x02

#pragma pack(push, 2)

/** 
  * @brief  сборка информации о памяти
  */
typedef struct
{
  uint32_t wr_ptr;        // +0
  uint32_t rd_ptr;        // +4
  uint32_t full_ptr;      // +8
} type_MEM_PART_REPORT;   // 12

/** 
  * @brief  структура хранения настроек отдельной части памяти
  */
typedef struct
{
  uint8_t self_num;
  uint32_t write_ptr, read_ptr;
  uint32_t full_frame_num;
  uint32_t start_frame_num, finish_frame_num;
  uint8_t frame_mode, mode;
  type_MEM_PART_REPORT report;
} type_MEM_PART_CONTROL;

typedef struct{
  uint32_t write_ptr;   //+0
  uint32_t read_ptr;    //+4
} type_MEM_PART_CFG;    // 8

typedef struct{
  union{
    typeSingleFrameStruct frame;    
    struct{
      type_SingleFrame_Header header; 		//+0
      type_MEM_PART_CFG full_cfg;         //+12
      type_MEM_PART_CFG part_cfg[13];     //+20
      uint8_t reserve[2];                 //+124
      uint16_t crc16;							        //+126
    }fields;
  }fr1;
} type_MEM_CFG;  // 128

typedef struct{
  typeSingleFrameStruct cfg_frames[EXT_MEM_SETTINGS_FRAME_NUM];
  int8_t cfg_fr_ld_status[EXT_MEM_SETTINGS_FRAME_NUM];
}type_MEM_LOADED_CFG;

/** 
  * @brief  структура хранения настроек и параметров внешней памяти
  */
typedef struct
{
  type_CY15B104QN_CONTROL cy15b104[CY15B104_MEM_NUM];
  type_MEM_PART_CONTROL part[PART_NUM];
  uint32_t read_ptr;
  uint8_t sat_id;
  uint8_t dev_id;
  uint16_t self_frame_num;
  type_MEM_CFG self_cfg;
  type_MEM_LOADED_CFG loaded_cfg;
  int8_t fram_load_status;
} type_MEM_CONTROL;

#pragma pack(pop)

//
int8_t ext_mem_init(type_MEM_CONTROL* mem_ptr, SPI_HandleTypeDef* spi_ptr, uint16_t sat_id, uint8_t dev_id);
int8_t ext_mem_get_mem_cfg_frames(type_MEM_CONTROL* mem_ptr, typeSingleFrameStruct *mem_cfg_frame_1_ptr);
int8_t ext_mem_form_cfg_frame(type_MEM_CONTROL* mem_ptr);
int8_t ext_mem_full_update_cfg(type_MEM_CONTROL* mem_ptr);
int8_t ext_mem_part_update_cfg(type_MEM_CONTROL* mem_ptr, uint8_t part_num);
int8_t ext_mem_part_apply_cfg(type_MEM_CONTROL* mem_ptr, typeSingleFrameStruct fr1);
int8_t ext_mem_load_full_cfg_frames(type_MEM_CONTROL* mem_ptr);
int8_t ext_mem_get_cfg_frame(type_MEM_CONTROL* mem_ptr, uint8_t cfg_fr_num, typeSingleFrameStruct* cfg_fr_ptr);

void ext_mem_any_write(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t* buff);
void ext_mem_any_read(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t* buff);
void ext_mem_any_read_8b_block(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t offset, uint8_t* buff);
void ext_mem_any_line_read(type_MEM_CONTROL* mem_ptr, uint8_t* buff);
void ext_mem_any_line_read_8b_block(type_MEM_CONTROL* mem_ptr, uint8_t offset, uint8_t* buff);

int32_t ext_mem_check(type_MEM_CONTROL* mem_ptr, uint8_t symbol);
int32_t ext_mem_wr_param(type_MEM_CONTROL* mem_ptr, uint8_t *param, uint8_t settings_frame_num);
int32_t ext_mem_rd_param(type_MEM_CONTROL* mem_ptr, uint8_t *param, uint8_t settings_frame_num);
void ext_mem_wr_data_frame(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t *frame);
void ext_mem_rd_data_frame(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t *frame);
void ext_mem_rd_data_frame_8b_block(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t offset, uint8_t *frame);
void ext_mem_wr_frame_to_part(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t part_num);
void ext_mem_rd_frame_from_part(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t part_num);
void ext_mem_rd_frame_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t fr_addr, uint8_t part_num);
void ext_mem_wr_frame_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t fr_addr, uint8_t part_num);
void ext_mem_rd_buff_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint8_t *buff, uint16_t len, uint16_t fr_addr);
void ext_mem_wr_buff_to_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint8_t *buff, uint16_t len, uint16_t fr_addr);
void ext_mem_full_erase(type_MEM_CONTROL* mem_ptr, uint8_t symbol);
void ext_mem_format_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num);
int8_t ext_mem_set_wr_ptr_for_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint32_t wr_ptr);
int8_t ext_mem_set_rd_ptr_for_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint32_t rd_ptr);
void ext_mem_read_from_part_8b(type_MEM_CONTROL* mem_ptr, uint8_t offset, uint8_t *frame_part, uint8_t part_num);
type_MEM_PART_REPORT ext_mem_get_part_report(type_MEM_CONTROL* mem_ptr, uint8_t part_num);
//
uint32_t part_rel_init(type_MEM_PART_CONTROL* part_ptr, uint8_t self_num, uint8_t mode, uint16_t full_rel_vol, uint16_t rel_vol, uint32_t start_frame_addr);
uint32_t part_const_init(type_MEM_PART_CONTROL* part_ptr, uint8_t self_num, uint8_t mode, uint16_t const_vol, uint32_t start_frame_addr);
uint16_t part_get_free_volume_in_percentage(type_MEM_PART_CONTROL* part_ptr);
uint8_t part_wr_rd_ptr_calc(type_MEM_PART_CONTROL* part_ptr, uint8_t mode);
uint32_t part_get_full_frame_count(type_MEM_PART_CONTROL* part_ptr);
uint32_t part_get_wr_ptr(type_MEM_PART_CONTROL* part_ptr);
uint32_t part_get_rd_ptr(type_MEM_PART_CONTROL* part_ptr);
type_MEM_PART_REPORT part_get_report(type_MEM_PART_CONTROL* part_ptr);

#endif
