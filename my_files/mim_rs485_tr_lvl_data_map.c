/**
 * @file mim_rs485_tr_lvl.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-09-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "mim_rs485_tr_lvl_data_map.h"

//*** Описание информационной части

void mim_tr_inf_init(type_MIM_INF_ID_DATA* data_ptr)
{
    uint8_t cmd_num = 0;
    //
    cmd_num = mim_tr_inf_set_unit(  cmd_num, data_ptr, MIM_INF_ID_SHORT_TMI_RD,      	MIM_INF_LVL_MODE_RD, 16,  0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_TMI_RD,            	MIM_INF_LVL_MODE_RD, 128, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_SET_TIME,          	MIM_INF_LVL_MODE_WR, 6,   0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_SET_SETTINGS,      	MIM_INF_LVL_MODE_WR, 16 , 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_READ_SYSTEM_INFO,  	MIM_INF_LVL_MODE_RD, 256, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_SET_EXP_SETTINGS,  	MIM_INF_LVL_MODE_WR, 32, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_READ_EXP_SETTINGS, 	MIM_INF_LVL_MODE_RD, 32, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_READ_EXP_TMI,      	MIM_INF_LVL_MODE_RD, 128, 0);
		//
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_TX_CFG_REQ,        MIM_INF_LVL_MODE_WR, 6,   0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_TX_CFG_RD,       		MIM_INF_LVL_MODE_RD, 31 , 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_TX_DATA_WR,       	MIM_INF_LVL_MODE_WR, 287, 0);
		//
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_RX_CFG_REQ,    		MIM_INF_LVL_MODE_WR, 6,   0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_RX_CFG_RD,       		MIM_INF_LVL_MODE_RD, 326, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_RX_CFG_WR,         	MIM_INF_LVL_MODE_WR, 326, 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_RX_DATA_RD,  				MIM_INF_LVL_MODE_RD, 4,   MIM_INF_LVL_MODE_RD);
		//
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_S_TX_CFG_REQ,    	MIM_INF_LVL_MODE_WR, 6,   0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_S_TX_CFG_RD, 				MIM_INF_LVL_MODE_RD, 32 , 0);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_S_TX_DATA_WR,     	MIM_INF_LVL_MODE_WR, 288, 0);
		//
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_EXP_DATA_REQ,      MIM_INF_LVL_MODE_WR, 6, 0);
		cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_EXP_DATA_RD,       	MIM_INF_LVL_MODE_RD, 128, 0);
		
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_MFR_DATA_RD,       	MIM_INF_LVL_MODE_RD, 1014, MIM_INF_LVL_MODE_RD);
    cmd_num = mim_tr_inf_set_unit(++cmd_num, data_ptr, MIM_INF_ID_MFR_SEGM_DATA_WR,  	MIM_INF_LVL_MODE_WR, 64, 0);
    // ...
		// контроль инициализации
		printf("%s: MIM TR LVL cmd init (normal is %d): %d\n", now(), MIM_INF_ID_NUM, ++cmd_num);
}

/**
 * @brief Установка отдельного id
 * 
 * @param data_ptr 
 * @param ID 
 * @param mode 
 * @param init_flag флаг после инициализации (для инициализации данных)
 */
uint8_t mim_tr_inf_set_unit(uint8_t cmd_num, type_MIM_INF_ID_DATA* data_ptr, uint8_t unit_id, uint8_t mode, uint16_t default_len, uint8_t init_flag)
{
    if (cmd_num < MIM_INF_ID_NUM) {
        if(unit_id < MIM_INF_ID_PULL_NUM){
            data_ptr->unit[cmd_num].self_id = unit_id;
            data_ptr->unit[cmd_num].mode = mode;
            data_ptr->unit[cmd_num].data_len = (default_len <= MIM_TR_DATA_SIZE_MAX) ? default_len+1 : 0;
            memset(data_ptr->unit[cmd_num].data.array, 0xFE, sizeof(data_ptr->unit[cmd_num].data));
            data_ptr->unit[cmd_num].flags = (init_flag == 0) ? 0x00 : init_flag;
            data_ptr->id_map[unit_id] = cmd_num;
        }
    }
    return cmd_num;
}

/**
 * @brief установка данных для передачи по запросу
 * 
 * @param mim_inf_ptr 
 * @param id id транзакции
 * @param data данные для передачи не включая id
 * @param data_len длина данных информационной части без id
 */
void mem_tr_inf_receive_id_data(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t* data, uint16_t data_len)
{
    uint8_t cmd_num = data_ptr->id_map[id];
    if ((cmd_num < MIM_INF_ID_NUM) && (id < MIM_INF_ID_PULL_NUM)) {
        data_ptr->unit[cmd_num].data_len = MIN(data_len+1, MIM_TR_DATA_SIZE_MAX);
        data_ptr->unit[cmd_num].data.fields.id = id;
        memcpy(data_ptr->unit[cmd_num].data.fields.array, data, data_len);
    }
}

/**
 * @brief Устанавливает указанный флаг
 * 
 * @param data_ptr 
 * @param id 
 * @param flag флаг для установки
 */
void mem_tr_inf_set_unit_flag(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t flag)
{
    uint8_t cmd_num = data_ptr->id_map[id];
    if ((cmd_num < MIM_INF_ID_NUM) && (id < MIM_INF_ID_PULL_NUM)) {
        data_ptr->unit[cmd_num].flags |= flag;
    }
}

/**
 * @brief Запрос наличия конкретного флага. После запроса флаг зануляется.
 * 
 * @param data_ptr 
 * @param id 
 * @param flag_mask маска для флага
 * @return uint8_t возвращает флаг
 */
uint8_t mem_tr_inf_get_unit_flag(type_MIM_INF_ID_DATA* data_ptr, uint8_t id, uint8_t flag_mask)
{
    uint8_t cmd_num = data_ptr->id_map[id];
    uint8_t ret_flag = 0;
    if ((cmd_num < MIM_INF_ID_NUM) && (id < MIM_INF_ID_PULL_NUM)) {
        ret_flag = data_ptr->unit[cmd_num].flags & flag_mask;
        data_ptr->unit[cmd_num].flags &= ~flag_mask;
    }
    return (ret_flag) ? 1 : 0;
}
