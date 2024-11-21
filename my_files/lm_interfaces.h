#ifndef _LM_INTERFACES_H_
#define _LM_INTERFACES_H_

//*** include files ***//
// #include "lm.h"
#include "frames.h"
#include "canv.h"
#include "flash_lib.h"
#include "main.h"
#include "lm_interfaces_data.h"
#include "pl_kkd.h"
//#include "pl_sol.h"
//#include "pl_brk.h"

//*** defines ***//
// LM setup
//***General setup
#define CAN_BROADCAST_ID 0x0F
//***VarId setup
#define ID_IVAR_PRG_MEM       0
#define ID_IVAR_RAM_MEM       1
#define ID_IVAR_CMD           2
#define ID_IVAR_CMD_STAT      3
#define ID_IVAR_CMDREG        4
#define ID_IVAR_TMI           5
#define ID_IVAR_PARAM         6
#define ID_IVAR_EXTMEM        7
#define ID_IVAR_FT_WR         8
#define ID_IVAR_DBG           12
#define ID_IVAR_BRD           13
#define ID_IVAR_FLASH_LOAD    14  //для прошивки через CAN

#define ID_IVAR_POOL_LEN      12
//***Cmds defines
#define CMD_INIT_LM                     0x00
#define CMD_DBG_LED_TEST                0x10
//
#define CMD_POOL_LEN          32
//
#define CMD_NO_ONE            -1
// default cmd statuses
#define CMD_STATUS_CLEAR           0x00
#define CMD_STATUS_START           0x01
#define CMD_STATUS_FINISH          0x7F
#define CMD_STATUS_CANCEL          0xFF
// default cmd
#define CMD_CLEAR                  0x00
#define CMD_START                  0x01
#define CMD_CANCEL                 0xFF
//***CmdsReg setup
// стандартные команды - 16 байт
#define CMDREG_TIME_0                   0x00
#define CMDREG_TIME_1                   0x01
#define CMDREG_TIME_2                   0x02
#define CMDREG_TIME_3                   0x03

#define CMDREG_CONST_MODE               0x04

#define CMDREG_PART_MEM_INIT_KEY        0x05
#define CMDREG_PART_MEM_INIT            0x06

#define CMDREG_PART_MEM_SET_RD_PTR      0x07
#define CMDREG_PART_MEM_SET_RD_PTR_0    0x08  
#define CMDREG_PART_MEM_SET_RD_PTR_1    0x09  
#define CMDREG_PART_MEM_SET_RD_PTR_2    0x0A  
#define CMDREG_PART_MEM_SET_RD_PTR_3    0x0B

//запись полётного задания
#define CMDREG_WRITE_FT_TO_MEM     0x0C
#define CMDREG_READ_FT_FROM_MEM    0x0D
#define CMDREG_READ_FT_FROM_REGS   0x0E
#define CMDREG_RUN_FT              0x0F

// команды подсистемы
#define CMDREG_LM_MODE             0x10
#define CMDREG_PL_PWR_SW           0x11
#define CMDREG_PL_PWR_SW_ON        0x12
#define CMDREG_PL_PWR_SW_OFF       0x13
#define CMDREG_PL_FT_DEFAULT_0     0x14
#define CMDREG_PL_FT_DEFAULT_1     0x15
#define CMDREG_PL_FT_STOP          0x16

#define CMDREG_PL_KKD_GET_TMI         0x20
#define CMDREG_PL_KKD_GET_DATA        0x21
#define CMDREG_PL_KKD_SET_INTER_S_0   0x22
#define CMDREG_PL_KKD_SET_INTER_S_1   0x23
#define CMDREG_PL_KKD_SET_RD_PTR_0    0x24
#define CMDREG_PL_KKD_SET_RD_PTR_1    0x25
#define CMDREG_PL_KKD_SET_RD_PTR_2    0x26
#define CMDREG_PL_KKD_SET_RD_PTR_3    0x27
#define CMDREG_PL_KKD_SYNCH_TIME      0x28

#define CMDREG_PL_AZNV_SET_SETTINGS    0x40
#define CMDREG_PL_AZNV_SET_MASK        0x42
#define CMDREG_PL_AZNV_SET_MS_TIME     0x43
#define CMDREG_PL_AZNV_SOFT_RESET      0x44
#define CMDREG_PL_AZNV_MODBUS_CMD      0x46
#define CMDREG_PL_AZNV_REG_TMI         0x50
#define CMDREG_PL_AZNV_GET_TMI         0x51
#define CMDREG_PL_AZNV_EXTRA_TMI       0x52
#define CMDREG_PL_AZNV_GET_SINGL_FRAME 0x53
#define CMDREG_PL_AZNV_GET_ALL_MSG     0x55

// //
// #define CMDREG_PL_SOL_RUN_CG        0x20
// #define CMDREG_PL_SOL_STOP_CG       0x21
// #define CMDREG_PL_SOL_GET_TMI       0x22
// #define CMDREG_PL_SOL_WINDOW_CTRL   0x23
// #define CMDREG_PL_SOL_START_EXP_0   0x24
// #define CMDREG_PL_SOL_START_EXP_1   0x25
// #define CMDREG_PL_SOL_GET_FRR       0x26
// #define CMDREG_PL_SOL_GET_FR        0x27
// #define CMDREG_PL_BRK_GET_TMI       0x28
// //
// #define CMDREG_PL_SOL_SEND_MB_ADDR    0x2A
// #define CMDREG_PL_SOL_SEND_MB_LEN     0x2B
// #define CMDREG_PL_SOL_SEND_MB_DATA_0  0x2C
// #define CMDREG_PL_SOL_SEND_MB_DATA_1  0x2D
// #define CMDREG_PL_SOL_SEND_MB_DATA_2  0x2E
// #define CMDREG_PL_SOL_SEND_MB_DATA_3  0x2F


//
#define CMDREG_SOFT_RESET          0x30 // выполняется только при записи значения CMDREG_SOFT_RESET_VALUE
//
#define CMDREG_DBG_LED             0x40
//
#define CMDREG_POOL_LEN            0x80
// cmd reg constants
#define CMDREG_SOFT_RESET_VALUE    0xA5
//

//***CAN_setup
/***   bit time   ***/

#define APB_PEREPHERIAL_CLOCK   36000000
#define CAN_BAUDRATE            1000000
#define CAN_SETUP_BTR           (0x01210000 | (APB_PEREPHERIAL_CLOCK/CAN_BAUDRATE/6 - 1))  //6=1+3+2, 1-SYNC_SEQ, 3-BS, 2-BS2

#define CAN_AFLG_READ_WRITE      0
//*** structures ***//


/*-----------Additional struct----------*/


/**
  * @brief  структура для хранения переменных управления командами (продолжительными со статусами, IdVar = 0х02)
  */
typedef struct
{ 
	uint32_t start_time_s, time_ms, point_time_ms;
	uint32_t main_counter;
	uint8_t ena;
} type_CMD_CONTROL;


/*------ Interface variables --------*/

#pragma pack(push, 2)
/**
  * @brief  All command
  */
typedef struct {
  uint8_t array[CMD_POOL_LEN];  //+0
} type_IVar_Cmds;//CMD_CNT

/**
  * @brief  All command statuses
  */
typedef struct {
  uint8_t array[CMD_POOL_LEN];  //+0
} type_IVar_Cmds_Statuses;//CMD_CNT

/**
  * @brief  All command registers
  */
typedef struct {
  uint8_t array[CMDREG_POOL_LEN];  //+0
} type_IVar_CmdRegisters;//CMDREG_POOL_LEN

/**
  * @brief  TMI data fields: contain data in 128-bytes form
  */
typedef struct {
  type_LM_Beacon_Frame beacon;                //+0
  type_LM_TMI_Data_Frame tmi;                 //+128
  //
  type_LM_GEN_TMI_Frame gen_tmi;              //+256
  type_LM_LOAD_PARAM_Frame load_parameters;   //+384
  //
  type_LM_CFG_Frame loaded_cfg;               //+512
  type_LM_CFG_Frame cfg_to_save;              //+640
  //
  typePL_KKD_TMI_Frame pl_kkd_tmi;            //+768
  typeSingleFrameStruct pl_kkd_data;          //+896
  //
  typeSingleFrameStruct pl_aznv_reg_tmi;          //+1024
  typeSingleFrameStruct pl_aznv_base_tmi;         //+1152
  typeSingleFrameStruct pl_aznv_add_tmi;          //+1280
  typeSingleFrameStruct pl_aznv_single_data;      //+1280
  typeSingleFrameStruct pl_aznv_array_data_head;  //+1408
  typeSingleFrameStruct pl_aznv_array_data_body;  //+1536
  //
  // typePL_SOL_FRR_Frame pl_sol_frr_tmi;        //+1024
  // typePL_SOL_FRR_Frame pl_sol_fr_tmi;         //+1152
  // //
} type_IVar_TMI;                              //1280

/**
  * @brief  Parameteres data
  */
typedef struct {
  // type_LM_Parameters_Frame param;  //+0
  uint8_t tmp[128]; //TODO:вернуть данные структуры при описании работы модуля сопряжения
} type_IVar_Param;//128

/**
  * @brief  External memory data in 128-bytes form
  */
typedef struct {
  uint8_t External_Mem_Full_Frame[128];     //+0
  uint8_t External_Mem_PART[PART_NUM][128]; //+128
} type_IVar_ExtMem; //128 + PART_NUM*128

/**
  * @brief  IVar for Flight Tasks
  */
typedef struct {
	uint8_t ft[16][FT_LEN_STEP][FT_STEP_LEN_BYTE];
} type_IVar_FT_Interface; //4224

/**
  * @brief  debug data
  */
typedef struct {
  uint8_t debug_data[128];  //+0
} type_IVar_DBG_Data; //128

#pragma pack(pop) //default


/*----------------------------------------*/
typedef struct
{
  //
  type_IVar_Cmds cmd;
  type_IVar_Cmds_Statuses cmd_status;
  uint8_t cmd_to_check[CMD_POOL_LEN];
  uint16_t cmd_flg;
  type_CMD_CONTROL cmd_ctrl[CMD_POOL_LEN];  // параметры для управления долгими командами
  //
  type_IVar_CmdRegisters cmdreg;
  uint8_t cmdreg_to_check[CMDREG_POOL_LEN];
  uint16_t cmdreg_flg;
  //
  type_IVar_Param parameters;
  //
  type_IVar_TMI tmi_data;
  //
  type_IVar_ExtMem ext_mem;
  //
  type_IVar_FT_Interface ft_write;
  //
  type_IVar_DBG_Data dbg_data;
  //
  type_IVar_DBG_Data brd_cast_data;
  //
  typeRegistrationRec *reg_rec_ptr;
  CAN_TypeDef *can1_ptr, *can2_ptr;
  uint16_t frame_num;
} type_LM_INTERFACES;

//*** functions prototypes ***//
uint16_t interfaces_init(type_LM_INTERFACES* lm_in_ptr, uint8_t id_dev);
typeRegistrationRec _reg_rec_setup(void *VarPtr, uint32_t VarLeng, void (*CallBackProc)(CAN_TypeDef *can_ref, typeIdxMask id, uint16_t leng, int state), uint8_t ivar_id, uint8_t access_flgs);
//работа с командами
void cmd_process_cb(type_LM_INTERFACES* lm_in_ptr, uint16_t cmd_num);
int16_t cmd_check_to_process(type_LM_INTERFACES* lm_in_ptr);
void cmd_set_status(type_LM_INTERFACES* lm_in_ptr, uint16_t cmd_num, uint8_t cmd_status);
//работа с командыми регистрами
void cmdreg_process_cb(type_LM_INTERFACES* lm_in_ptr, uint16_t cmd_num);
int16_t cmdreg_check_to_process(type_LM_INTERFACES* lm_in_ptr);
// посторонние функции
uint16_t RegisterAllVars(type_LM_INTERFACES* lm_in_ptr, uint8_t id_dev);
void interface_cb_registration(type_LM_INTERFACES* lm_in_ptr, uint8_t mode, void (*CallBackProc)(CAN_TypeDef *can_ref, typeIdxMask id, uint16_t leng, int state));

#endif
