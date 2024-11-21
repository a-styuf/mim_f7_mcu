#ifndef _LM_INTERFACES_DATA_H_
#define _LM_INTERFACES_DATA_H_

//*** Includes ***//
#include "crc16.h"
#include "clock.h"
#include "frames.h"
#include "ext_mem.h"
#include "flight_task.h"

//*** Defines ***//
// определитель кадров (метка для визуального или автоматического поиска кадров)
#define FRAME_MARK                    0x0FF1
// типы кадров 
#define SINGLE_FRAME_TYPE             0x00
#define ARCH_HEADER_FRAME_TYPE        0x01
#define ARCH_BODY_FRAME_TYPE          0x02
// типы данных в кадрах
#define DATA_TYPE_BEACON              0x80
#define DATA_TYPE_TMI                 0x81
#define DATA_TYPE_GEN_TMI             0x82
//
//
#define DATA_TYPE_LOAD_PARAM          0x8A

#define DATA_TYPE_LM_CONFIG           0x8F



//*** General parts of frames ***//
#pragma pack(push, 2)

/**
  * @brief  struct to store PL-power information
  */
typedef struct{
  uint8_t voltage;
  uint8_t current;
} type_POWER; //2

//***  Frame templates  ***//

/**
  * @brief  Beacon data in 128-bytes single_data form
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  //
  uint16_t lm_status; //+12
  uint16_t pl_status; //+14
  uint8_t lm_temp; //+16
  uint8_t pl_power_switches; //+17
  //
  uint8_t filler[108]; //+18
  //
  uint16_t crc16; //+126
} type_LM_Beacon_Frame; //128

/**
  * @brief  TMI data in 128-bytes single_data form
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  // 0-МС, 1-4 - ПН1-4, 5-6 - ПН5-6
  uint16_t pl_status[6];    //+12
  type_POWER pwr_inf[7];    //+24
  uint8_t temp[2];          //+38
  //
  type_MEM_PART_REPORT mem_report[9]; //+40
  //
  typeFTReport ft_report;      //+112
  //
  uint8_t filler[6];       //+120
  //
  uint16_t crc16;           //+126
} type_LM_TMI_Data_Frame;   //128

/**
  * @brief  Full LM TMI data in 128-bytes single_data form
  * @note   aLL system reports are contained in 18-bytes fields
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  // 0-МС, 1-ПН1.1A, 2-ПН1.1В, 3-ПН1.2, 4-ПН2.0, 5-ПН_ДКР
  uint8_t pl_report[6][18]; //+12
  uint16_t rsrv[3]; // +120
  //
  uint16_t crc16; //+126
} type_LM_GEN_TMI_Frame; //128

/**
  * @brief  Load parameters of LM
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  uint16_t version[3];          //+12
  int16_t pwr_init_report;      //+18
  int16_t tmp_init_report;      //+20
  int16_t cycl_init_report;     //+22
  int16_t int_init_report;      //+24
  int16_t ext_mem_init_report;  //+26
  int16_t cfg_load_report;      //+28
  int16_t flash_part_num;      //+30
  //
  uint8_t filler[94];           //+32
  //
  uint16_t crc16;               //+126
} type_LM_LOAD_PARAM_Frame;      //128

/**
  * @brief  PL interface answer
  * @note   answer from PL from UART must be less than 116-byte
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  //
  uint8_t data[116]; //+10
  //
  uint16_t crc16; //+126
} type_PL_INT_data; //128

/**
  * @brief  Parameters data
  */
typedef struct {
  type_SingleFrame_Header header;   //+0
  //
  uint8_t filler[116];              //+10
  //
  uint16_t crc16;                   //+126
} type_LM_Parameters_Frame;         //128


/**
  * @brief  Config parameters to save in special memory part
  * @note   aLL system cfg are contained in 18-bytes fields
  */
typedef struct {
  type_SingleFrame_Header header; //+0
  // 
  uint8_t pl_cfg[6][16];          //+12
  uint8_t filler[18];             //+108
  //
  uint16_t crc16;                 //+126
} type_LM_CFG_Frame;              //128

#pragma pack(pop)


//*** Function prototypes ***//
void fill_beacon_const_mode(type_LM_Beacon_Frame* frame_ptr);
void fill_tmi_const_mode(type_LM_TMI_Data_Frame* frame_ptr);
#endif
