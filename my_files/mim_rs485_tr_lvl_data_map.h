#ifndef _MIM_RS485_TR_LVL_DATA_MAP_H_
#define _MIM_RS485_TR_LVL_DATA_MAP_H_

#include <stdio.h>
#include "usart.h"
#include "ivar_data.h"
#include "my_gpio.h"
#include "crc16.h"
#include "debug.h"
#include "clock.h"

#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#define MIM_TR_FRAME_SIZE_MAX       (1024)
#define MIM_TR_OVERHEAD_SIZE_MAX    (9)
#define MIM_TR_DATA_SIZE_MAX        (MIM_TR_FRAME_SIZE_MAX - MIM_TR_OVERHEAD_SIZE_MAX)

#define MIM_INF_LVL_MODE_WR (1<<0)
#define MIM_INF_LVL_MODE_RD (1<<1)

typedef enum{
    MIM_INF_ID_SHORT_TMI_RD = 0,
    MIM_INF_ID_TMI_RD, 
    MIM_INF_ID_SET_TIME, 
    MIM_INF_ID_SET_SETTINGS, 
    MIM_INF_ID_READ_SYSTEM_INFO, 
    MIM_INF_ID_SET_EXP_SETTINGS, 
    MIM_INF_ID_READ_EXP_SETTINGS,
    MIM_INF_ID_READ_EXP_TMI,
    //
    MIM_INF_ID_TX_CFG_REQ = 16,
    MIM_INF_ID_TX_CFG_RD, 
    MIM_INF_ID_TX_DATA_WR, 
    //
    MIM_INF_ID_RX_CFG_REQ, 
    MIM_INF_ID_RX_CFG_RD,
    MIM_INF_ID_RX_CFG_WR,
    MIM_INF_ID_RX_DATA_RD,
    //
    MIM_INF_ID_S_TX_CFG_REQ = 32,
    MIM_INF_ID_S_TX_CFG_RD,
    MIM_INF_ID_S_TX_DATA_WR,
    //
    MIM_INF_ID_EXP_DATA_REQ = 48,
    MIM_INF_ID_EXP_DATA_RD,
    //
    MIM_INF_ID_MFR_DATA_RD = 64,
    MIM_INF_ID_MFR_SEGM_DATA_WR,
    //
    MIM_INF_ID_PULL_NUM //! кол-во забронированного пространства id
} type_MIM_INF_UNIT_ID;

#define MIM_INF_ID_NUM 22  //! кол-во используемых команд

#pragma pack(push, 2)

typedef struct {
    uint8_t self_id;
    union{
        uint8_t array[MIM_TR_DATA_SIZE_MAX+1];
        struct{
            uint8_t id;
            uint8_t array[MIM_TR_DATA_SIZE_MAX];
        } fields;
        struct{
            uint8_t id;
            uint8_t redirect_id;
            uint8_t array[MIM_TR_DATA_SIZE_MAX-1];
        } redirect;
    } data;
    uint16_t data_len;
    uint8_t flags;
    uint8_t mode;
} type_MIM_INF_ID_DATA_UNIT;

#pragma pack(pop)

#pragma pack(push, 1)

/**
 * @brief структура для разбора данных команды на отправку данных (c ID)
 * 
 */
typedef union{
    uint8_t array[288];
    struct{
        uint8_t id;
        uint8_t array[287];
    } tr_lvl;
    struct{
        uint8_t id;             // +0
        uint16_t packet_type;   // +1
        uint16_t packet_size;   // +3
        uint8_t module;         // +5
        uint8_t immediate;      // +6
        uint32_t time_h;        // +7
        uint32_t time_l;        // +11
        typeIVarTxSet tx_frame; // +15
    } fl;
} type_MIM_INF_ID_TX_DATA_WR;

/**
 * @brief структура для разбора данных команды на отправку данных (c ID)
 * 
 */
typedef union{
    uint8_t array[288];
    struct{
        uint8_t id;
        uint8_t array[287];
    } tr_lvl;
    struct{
        uint8_t id;             // +0
        uint16_t packet_type;   // +1
        uint16_t packet_size;   // +3
        uint8_t module;         // +5
        uint8_t reserve[9];     // +6
        typeIVarTxSet tx_frame; // +15
    } fl;
} type_MIM_INF_ID_TX_CFG;

/**
 * @brief структура для разбора запроса конфигурации
 * 
 */
typedef union{
    uint8_t array[7];
    struct{
        uint8_t id;
        uint8_t array[6];
    } tr_lvl;
    struct{
        uint8_t id;             // +0
        uint16_t packet_type;   // +1
        uint16_t packet_size;   // +3
        uint8_t module;         // +5
        uint8_t reserve;        // +6
    } fl;
} type_MIM_INF_ID_CFG_REQ;

/**
 * @brief структура для разбора данных на конфигурацию приемника
 * 
 */
typedef union{
    uint8_t array[327];
    struct{
        uint8_t id;
        uint8_t array[326];
    } tr_lvl;
    struct{
        uint8_t id;             // +0
        uint16_t packet_type;   // +1
        uint16_t packet_size;   // +3
        uint8_t module;         // +5
        uint8_t reserve;        // +6
        typeIVarRxSet rx_set[4];   // +7
    } fl;
} type_MIM_INF_ID_RX_CFG;

/**
 * @brief структура для разбора данных на конфигурацию приемника
 * 
 */
typedef union{
    uint8_t array[307];
    struct{
        uint8_t id;
        uint8_t array[306];
    } tr_lvl;
    struct{
        uint8_t id;                 // +0
        uint16_t packet_type;       // +1
        uint16_t packet_size;       // +3
        typePktRx packet;           // +5
    } fl;
} type_MIM_INF_ID_RX_DATA;

typedef struct {
    type_MIM_INF_ID_DATA_UNIT unit[MIM_INF_ID_NUM];
    uint8_t id_map[MIM_INF_ID_PULL_NUM];
} type_MIM_INF_ID_DATA;

#pragma pack(pop)

void mim_tr_inf_init(type_MIM_INF_ID_DATA* data_ptr);
uint8_t mim_tr_inf_set_unit(uint8_t cmd_num, type_MIM_INF_ID_DATA* data_ptr, uint8_t unit_id, uint8_t mode, uint16_t default_len, uint8_t init_flag);
void mem_tr_inf_receive_id_data(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t* data, uint16_t data_len);
void mem_tr_inf_set_unit_flag(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t flag);
uint8_t mem_tr_inf_get_unit_flag(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t flag_mask);

#endif
