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

#include "mim_rs485_tr_lvl.h"

void mim_tr_init(type_MIM_TR_LVL_Interface* mim_tr_ptr, UART_HandleTypeDef* huart, GPIO_TypeDef* ch0_ena_bank, uint16_t ch0_ena_line, GPIO_TypeDef* ch1_ena_bank, uint16_t ch1_ena_line, TIM_HandleTypeDef* tim, type_MIM_INF_ID_DATA* data_map, uint8_t de_invert)
{
    mim_tr_ptr->huart = huart;
    mim_tr_ptr->ch0_ena = gpio_parameters_set(ch0_ena_bank, ch0_ena_line);
    mim_tr_ptr->ch1_ena = gpio_parameters_set(ch1_ena_bank, ch1_ena_line);
    mim_tr_ptr->used_rx_ch_de = MIM_TR_CH0;
    mim_tr_ptr->tim = tim;
    mim_tr_ptr->de_invert = de_invert;
    if (mim_tr_ptr->de_invert) {
        gpio_set(&mim_tr_ptr->ch0_ena, 1);
        gpio_set(&mim_tr_ptr->ch1_ena, 1);
    }
    else {
        gpio_set(&mim_tr_ptr->ch0_ena, 0);
        gpio_set(&mim_tr_ptr->ch1_ena, 0);
    }
    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);
    mim_tr_ptr->data_map = data_map;
    //
    segm_init(&mim_tr_ptr->segm);
    //
    mim_tr_ptr->rx_ptr = 0;
    mim_tr_ptr->rx_ready = 0;
    mim_tr_ptr->rx_packet_num = 0;
    memset(mim_tr_ptr->rx_buffer, 0x00, sizeof(mim_tr_ptr->rx_buffer));
    HAL_UART_Receive_IT(mim_tr_ptr->huart, &mim_tr_ptr->rx_buffer[mim_tr_ptr->rx_ptr], 1);
}

/**
 * @brief получения массива для передачи из структуры кадра type_MIM_TR_FRAME
 * 
 * @param mim_tr_ptr 
 * @return uint16_t 
 */
uint16_t mim_tr_form_tx_data(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    memset(mim_tr_ptr->tx_data, 0xFE, sizeof(mim_tr_ptr->tx_data));
    //
    mim_tr_ptr->tx_data[0] = mim_tr_ptr->tx_frame.fields.start_symb;
    //
    mim_tr_ptr->tx_data[1] = mim_tr_ptr->tx_frame.fields.rx_addr;
    mim_tr_ptr->tx_data[2] = mim_tr_ptr->tx_frame.fields.tx_addr;
    mim_tr_ptr->tx_data[3] = mim_tr_ptr->tx_frame.fields.flags;
    //
    *(uint16_t*)&mim_tr_ptr->tx_data[4] = __REV16(mim_tr_ptr->tx_frame.fields.data_len);
    mim_tr_ptr->tx_data[6] = mim_tr_ptr->tx_frame.fields.inf.fields.id;
    memcpy(&mim_tr_ptr->tx_data[7], mim_tr_ptr->tx_frame.fields.inf.fields.data, mim_tr_ptr->tx_frame.fields.data_len-1);
    //
    mim_tr_ptr->tx_data[6 + mim_tr_ptr->tx_frame.fields.data_len] = crc8ccitt(   
                                                                                (uint8_t*)&mim_tr_ptr->tx_data[1], 
                                                                                mim_tr_ptr->tx_frame.fields.data_len + 5,
                                                                                0x00
                                                                            );
    //
    mim_tr_ptr->tx_data[6 + mim_tr_ptr->tx_frame.fields.data_len + 1] = mim_tr_ptr->tx_frame.fields.stop_symb;
    //
    mim_tr_ptr->tx_data_size = 6 + mim_tr_ptr->tx_frame.fields.data_len + 2;
	return mim_tr_ptr->tx_data_size;
}


/**
 * @brief заполнение кадра на отправку
 * 
 * @param mim_tr_ptr 
 * @param flag флаги типа кадра
 * @param id id информационной части
 * @param data данные без id
 * @param data_len полные данные включая id информационной части
 */
void mim_tr_tx_frame_fill(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t flag, uint8_t id, uint8_t* data, uint16_t data_len)
{   
    mim_tr_ptr->tx_frame.fields.start_symb = MIM_TR_FRAME_START_SYMBOL;
    mim_tr_ptr->tx_frame.fields.rx_addr = mim_tr_ptr->tx_addr;
    mim_tr_ptr->tx_frame.fields.tx_addr = MIM_TR_FRAME_SELF_ADDR;
    mim_tr_ptr->tx_frame.fields.flags = flag;
    mim_tr_ptr->tx_frame.fields.data_len = MIN(data_len, MIM_TR_FRAME_SIZE_MAX);
    //
    mim_tr_ptr->tx_frame.fields.inf.fields.id = id;
    memcpy(mim_tr_ptr->tx_frame.fields.inf.fields.data, data, (mim_tr_ptr->tx_frame.fields.data_len - 1));
    // некорректное crc!!!
    mim_tr_ptr->tx_frame.fields.crc8 = crc8ccitt(   
                                                    (uint8_t*)&mim_tr_ptr->tx_frame.fields.rx_addr, 
                                                    mim_tr_ptr->tx_frame.fields.data_len + 5,
                                                    0x00
                                                );
    //
    mim_tr_ptr->tx_frame.fields.stop_symb = MIM_TR_FRAME_STOP_SYMBOL;
}

/**
 * @brief включение/отключение режима передачи с указанием канала
 * 
 * @param mim_tr_ptr 
 * @param val 1 - включение, 0 - отключение
 */
void mim_tr_set_de(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t val)
{
    uint8_t ch_re = 0;
	if (mim_tr_ptr->de_invert == 0) ch_re = (val&0x01) ? 0x01 : 0x00;
    else if (mim_tr_ptr->de_invert != 0) ch_re = (val&0x01) ? 0x00 : 0x01;
    switch(mim_tr_ptr->used_rx_ch_de){
        case(MIM_TR_CH0):
            gpio_set(&mim_tr_ptr->ch0_ena, ch_re);
            // gpio_set(&mim_tr_ptr->ch1_ena, ch_re);
        break;
        case(MIM_TR_CH1):
            gpio_set(&mim_tr_ptr->ch0_ena, ch_re);
            // gpio_set(&mim_tr_ptr->ch1_ena, ch_re);
        break;
    }
}

/**
 * @brief Отправка пакета через транспортный протокол
 * 
 * @param mim_tr_ptr 
 * @return int8_t 0 - ОК, <0 - not OK
 */
int8_t mim_tr_transmit_tx_frame(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
	volatile HAL_StatusTypeDef transmit_state = HAL_OK;
	int8_t ret_val = 0;

    mim_tr_set_de(mim_tr_ptr, 1);
    transmit_state = HAL_UART_Transmit_DMA(mim_tr_ptr->huart, mim_tr_ptr->tx_data, mim_tr_ptr->tx_data_size);
    switch(transmit_state){
        case HAL_OK:
            // printf("mim tr tx ok\n");
            ret_val = 0;
        break;	
        case HAL_BUSY:
            // printf("mim tr tx busy\n");
            ret_val = -1;
        break;
        case HAL_TIMEOUT:
            // printf("mim tr tx timeout\n");
            ret_val = -2;
        break;
        case HAL_ERROR:
            // printf("mim tr tx error\n");
            ret_val = -3;
        break;
    }
	if (ret_val < 0) mim_tr_set_de(mim_tr_ptr, 0);
    return ret_val;
}

/**
 * @brief Функция для вызова в прерывание по отправке пакета байта через UART
 * 
 * @param mb_i_ptr управляющая структура
 */
void mim_tr_it_tx_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
	mim_tr_set_de(mim_tr_ptr, 0);
}


/**
 * @brief обработчик прерывания от таймера о достижении интервала приема в ~3,5 байта данных
 * 
 * @param mim_tr_ptr
 */
void mim_tr_it_rx_timer_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    HAL_TIM_Base_Stop_IT(mim_tr_ptr->tim);
    HAL_UART_Abort_IT(mim_tr_ptr->huart);
    //
    if (mim_tr_it_check_rx_packet(mim_tr_ptr) == 0){
        mim_tr_ptr->rx_ready = 1;
        mim_tr_ptr->rx_packet_num += 1;
        // формирование ответа
        mim_tr_ptr->rx_id = mim_tr_ptr->rx_frame.fields.inf.fields.id;
        if (mim_tr_ptr->rx_frame.fields.rx_addr == MIM_TR_FRAME_SELF_ADDR) {
                switch(mim_tr_ptr->rx_frame.fields.flags){
                        case(MIM_TR_FRAME_FLAGS_CMD):
                            if (mim_tr_ptr->rx_id == MIM_INF_ID_MFR_SEGM_DATA_WR){
                                segm_put_new_frame(&mim_tr_ptr->segm, mim_tr_ptr->rx_frame.fields.inf.fields.data);
                                mim_tr_cmd_with_segm_data_proccess( mim_tr_ptr, 
                                                                    mim_tr_ptr->rx_id, 
                                                                    mim_tr_ptr->rx_frame.fields.inf.fields.data,
                                                                    __REV16(mim_tr_ptr->rx_frame.fields.data_len));
                            }
                            else{
                                mim_tr_cmd_proccess( mim_tr_ptr, 
                                                    mim_tr_ptr->rx_id, 
                                                    mim_tr_ptr->rx_frame.fields.inf.fields.data,
                                                    __REV16(mim_tr_ptr->rx_frame.fields.data_len));
                            }
                        break;
                        case(MIM_TR_FRAME_FLAGS_DATA):
                            mim_tr_rd_data_proccess( mim_tr_ptr, 
                                                    mim_tr_ptr->rx_id);
                        break;
                        default:
                            //TODO: ошибка некорректных флагов
                        break;
                }
        }
        else{
            //TODO: ошибка выхода за доступные адреса
        }
    }
    else{
        //TODO: ошибка выхода за доступные адреса
    }
    //
    mim_tr_ptr->rx_ptr = 0;
    HAL_UART_Receive_IT(mim_tr_ptr->huart, &mim_tr_ptr->rx_buffer[mim_tr_ptr->rx_ptr], 1);
}

/**
 * @brief обработка команд управления
 * 
 * @param mim_tr_ptr 
 */
void mim_tr_cmd_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id, uint8_t* rx_data, uint16_t rx_data_len)
{
    uint8_t uint8_arr[8] = {0};
    //
    mem_tr_inf_receive_id_data(mim_tr_ptr->data_map, rx_id, rx_data, rx_data_len);
    mem_tr_inf_set_unit_flag(mim_tr_ptr->data_map, rx_id, MIM_INF_LVL_FLAGS_CMD);
    //
    uint8_arr[0] = MIM_TR_FRAME_ACK;
    mim_tr_tx_frame_fill(mim_tr_ptr, MIM_TR_FRAME_FLAGS_CMD_ANSW, rx_id, uint8_arr, 2);
    mim_tr_form_tx_data(mim_tr_ptr);
    mim_tr_transmit_tx_frame(mim_tr_ptr);
}

/**
 * @brief обработка команд управления
 * 
 * @param mim_tr_ptr 
 */
void mim_tr_cmd_with_segm_data_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id, uint8_t* rx_data, uint16_t rx_data_len)
{
    uint8_t uint8_arr[8] = {0};
	//
    if(segm_check_new_data(&mim_tr_ptr->segm)){
        mem_tr_inf_receive_id_data(mim_tr_ptr->data_map, rx_id, mim_tr_ptr->segm.data, mim_tr_ptr->segm.data_len);
        mem_tr_inf_set_unit_flag(mim_tr_ptr->data_map, rx_id, MIM_INF_LVL_FLAGS_CMD);
    }
    //
    uint8_arr[0] = MIM_TR_FRAME_ACK;
    mim_tr_tx_frame_fill(mim_tr_ptr, MIM_TR_FRAME_FLAGS_CMD_ANSW, rx_id, uint8_arr, 2);
    mim_tr_form_tx_data(mim_tr_ptr);
    mim_tr_transmit_tx_frame(mim_tr_ptr);
}

/**
 * @brief Перенаправление данных, принятых в рамкам сегментированных данных на другой ID
 * 
 * @param mim_tr_ptr 
 * @return int8_t 1 - команда перенаправлена, 0 - ошибка
 */
int8_t mim_tr_redirect_segm_data_cmd(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    uint8_t cmd_num = mim_tr_ptr->data_map->id_map[MIM_INF_ID_MFR_SEGM_DATA_WR];
    uint8_t redirect_id = mim_tr_ptr->data_map->unit[cmd_num].data.redirect.redirect_id;
    if ((redirect_id < MIM_INF_ID_PULL_NUM) && ((redirect_id != MIM_INF_ID_MFR_SEGM_DATA_WR))){
        if (mim_tr_ptr->data_map->unit[cmd_num].data_len >= 2){
            mem_tr_inf_receive_id_data( mim_tr_ptr->data_map, 
                                        redirect_id, 
                                        mim_tr_ptr->data_map->unit[cmd_num].data.redirect.array, 
                                        mim_tr_ptr->data_map->unit[cmd_num].data_len-2);
            mem_tr_inf_set_unit_flag(mim_tr_ptr->data_map, redirect_id, MIM_INF_LVL_FLAGS_CMD);
            return 1;
        }
        else{
        }
    }
    else{
    }
    return 0;
}

/**
 * @brief обработка команд чтения данных
 * 
 * @param mim_tr_ptr 
 */
void mim_tr_rd_data_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id)
{
		uint8_t cmd_num = mim_tr_ptr->data_map->id_map[rx_id];
    mem_tr_inf_set_unit_flag(mim_tr_ptr->data_map, rx_id, MIM_INF_LVL_FLAGS_RD);
    //
    mim_tr_tx_frame_fill(mim_tr_ptr, MIM_TR_FRAME_FLAGS_DATA_ANSW, rx_id, mim_tr_ptr->data_map->unit[cmd_num].data.fields.array, mim_tr_ptr->data_map->unit[cmd_num].data_len);
    mim_tr_form_tx_data(mim_tr_ptr);
    mim_tr_transmit_tx_frame(mim_tr_ptr);
}

/**
 * @brief обработчик прерывания при принятии символа
 * 
 * @param mim_tr_ptr
 */
void mim_tr_it_rx_uart_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    if (mim_tr_ptr->rx_ptr < MIM_TR_FRAME_SIZE_MAX) {
        HAL_UART_Receive_IT(mim_tr_ptr->huart, &mim_tr_ptr->rx_buffer[++mim_tr_ptr->rx_ptr], 1);
    }
    else{
        mim_tr_it_rx_error_cb(mim_tr_ptr);
        return;
    }
    //
    if(mim_tr_ptr->tim->Instance->CNT != 0){
        mim_tr_ptr->tim->Instance->CNT = 0;
    }
    else{
        HAL_TIM_Base_Start_IT(mim_tr_ptr->tim);
    }
}

/**
 * @brief 
 * 
 * @param mim_tr_ptr 
 * @return uint8_t 0 - rx_frame is OK
 */
uint8_t mim_tr_it_check_rx_packet(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{   
    uint16_t len = mim_tr_ptr->rx_ptr;
    volatile uint8_t crc8 = 0;
    if(mim_tr_ptr->rx_buffer[0] == MIM_TR_FRAME_START_SYMBOL){
        mim_tr_ptr->rx_data_len = __REV16(*(uint16_t*)&mim_tr_ptr->rx_buffer[4]);
        if(mim_tr_ptr->rx_data_len == len - 8){
            crc8 = crc8ccitt(&mim_tr_ptr->rx_buffer[1], len-3, 0x00);
            if (crc8 == mim_tr_ptr->rx_buffer[len-2]){
                memcpy((uint8_t*)&mim_tr_ptr->rx_frame, mim_tr_ptr->rx_buffer, mim_tr_ptr->rx_ptr);
                mim_tr_ptr->rx_frame.fields.crc8 = mim_tr_ptr->rx_buffer[len-2];
                mim_tr_ptr->rx_frame.fields.stop_symb = mim_tr_ptr->rx_buffer[len-1];
                mim_tr_ptr->tx_addr = mim_tr_ptr->rx_frame.fields.tx_addr;
            }
            else{
                return 3;
            }
        }
        else{
            return 2;
        }
    }
    else{
        return 1;
    }
    return 0;
}

/**
 * @brief обработчик прерывания при принятии символа
 * 
 * @param mim_tr_ptr
 */
void mim_tr_it_rx_error_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    mim_tr_ptr->rx_ptr = 0;
		mim_tr_set_de(mim_tr_ptr, 0);
    HAL_TIM_Base_Stop_IT(mim_tr_ptr->tim);
    HAL_UART_Abort(mim_tr_ptr->huart);
    HAL_UART_Receive_IT(mim_tr_ptr->huart, &mim_tr_ptr->rx_buffer[mim_tr_ptr->rx_ptr++], 1);
}

/**
 * @brief проверка наличия пришедшей команды
 * 
 * @param mem_tr 
 * @return int16_t >= 0 - id команды, < 0 - отсутствует
 */
int16_t mim_tr_get_processed_cmd_id(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    uint32_t i = 0;
    for(i = 0; i < MIM_INF_ID_PULL_NUM; i++){
        if (mem_tr_inf_get_unit_flag(mim_tr_ptr->data_map, i, MIM_INF_LVL_FLAGS_CMD)){
            return i;
        }
    }
    return -1;
}

/**
 * @brief проверка наличия прочитанных данных
 * 
 * @param mem_tr 
 * @return int16_t >= 0 - id данных, < 0 - отсутствует
 */
int16_t mim_tr_get_processed_data_id(type_MIM_TR_LVL_Interface* mim_tr_ptr)
{
    uint32_t i = 0;
    for(i = 0; i < MIM_INF_ID_PULL_NUM; i++){
        if (mem_tr_inf_get_unit_flag(mim_tr_ptr->data_map, i, MIM_INF_LVL_FLAGS_RD)){
            return i;
        }
    }
    return -1;
}
