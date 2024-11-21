/**
 * @file segmented_data.c
 * @author your name (you@domain.com)
 * @brief работа с сегментированными данными для сборки полного пакета
 * @version 0.1
 * @date 2023-11-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "segmented_data.h"

/**
 * @brief инициализация структуры приема сегментированных данных
 * 
 * @param segm_ptr 
 */
void segm_init(type_SEGM* segm_ptr)
{
	memset((uint8_t*)segm_ptr->last_frame.array, 0x00, sizeof(type_SEGM_Frame));
	memset((uint8_t*)segm_ptr->frame_array, 0x00, sizeof(type_SEGM_Frame)*SEGM_DATA_MAX_FRAMES);
	memset((uint8_t*)segm_ptr->frame_status, 0x00, SEGM_DATA_MAX_FRAMES);
	memset((uint8_t*)segm_ptr->data, 0x00, sizeof(segm_ptr->data));
	segm_ptr->expected_segm_vol = 0x00;
	segm_ptr->expected_seq_num = 0xFFFFFFFF;
	segm_ptr->status = 0x00; 
	segm_ptr->err_cnter = 0x00; 
	segm_ptr->data_ready = 0x0;
	segm_ptr->data_len = 0x0;
	segm_ptr->new_data_ready = 0x0;
	segm_ptr->data_cnter = 0x0;
	segm_ptr->frame_cnter = 0x0;
}

void segm_reset_state(type_SEGM* segm_ptr)
{
	segm_ptr->status = 0x00; 
	segm_ptr->err_cnter = 0x00; 
	segm_ptr->data_len = 0x0;
	segm_ptr->data_cnter = 0x0;
	segm_ptr->frame_cnter = 0x0;
}

/**
 * @brief сброс данных для приема нового пакета данных
 * 
 * @param segm_ptr 
 */
void segm_refresh(type_SEGM* segm_ptr)
{
	memset((uint8_t*)segm_ptr->frame_array, 0x00, sizeof(type_SEGM_Frame)*SEGM_DATA_MAX_FRAMES);
	memset((uint8_t*)segm_ptr->data, 0x00, sizeof(segm_ptr->data));
	memset((uint8_t*)segm_ptr->frame_status, 0x00, SEGM_DATA_MAX_FRAMES);
}

/**
 * @brief прием нового сегмента данных
 * 
 * @param segm_ptr 
 */
void segm_put_new_frame(type_SEGM* segm_ptr, uint8_t* frame)
{
	if (segm_check_frame(segm_ptr, frame)){
		segm_check_full_frames(segm_ptr);
	}
}

/**
 * @brief проверка и раскладывание пакетов
 * 
 * @param segm_ptr 
 * @param frame 
 * @return int8_t 
 */
int8_t segm_check_frame(type_SEGM* segm_ptr, uint8_t* frame)
{
	type_SEGM_Frame tmp_frame;
	uint8_t crc8 = 0;
	//
	memcpy(tmp_frame.array, frame, sizeof(type_SEGM_Frame));
	//check label
	if (tmp_frame.field.label != 0xCDDE){
		segm_ptr->status |= SEGM_STATUS_SEGM_LABEL;
		return 0;
	}
	//check crc
	crc8 = crc8ccitt(tmp_frame.array, sizeof(type_SEGM_Frame) - 1, SEGM_FRAME_CRC8_INIT_VAL);
	if (crc8 != tmp_frame.field.crc8){
		segm_ptr->status |= SEGM_STATUS_SEGM_CRC;
		return 0;
	}
	//check data_len
	if (tmp_frame.field.data_len > SEGM_FRAME_DATA_LEN){
		segm_ptr->status |= SEGM_STATUS_DATA_LEN;
		return 0;
	}
	//check sequence number: if there is changing in seq_num, change expected_seq_num and raise new data receiving
	if (segm_ptr->expected_seq_num != tmp_frame.field.seq_num)	{
		segm_ptr->expected_seq_num = tmp_frame.field.seq_num;
		segm_ptr->expected_segm_vol = tmp_frame.field.segm_vol;
		//подготовка к приему новых данных
		segm_refresh(segm_ptr);
		segm_ptr->frame_array[tmp_frame.field.segm_num] = tmp_frame;
		segm_ptr->frame_status[tmp_frame.field.segm_num] = SEGM_FRAME_VALID;
		return 1;
	}
	else {
		if (segm_ptr->expected_segm_vol != tmp_frame.field.segm_vol){
			segm_ptr->status |= SEGM_STATUS_SEGM_VOL;
			return 0;
		}
		segm_ptr->frame_array[tmp_frame.field.segm_num] = tmp_frame;
		segm_ptr->frame_status[tmp_frame.field.segm_num] = SEGM_FRAME_VALID;
		return 1;
	}
}

/**
 * @brief проверка полного объема принятых сегментов данных:
 *   - в случае успеха формируется полный пакет информационных данных и выставляется флаг готовности данных
 *   - в случае неудачи флаг не выставляется
 * @param segm_ptr 
 * @return int8_t 1 - пакет собрался, 0 - пакет не собрался
 */
int8_t segm_check_full_frames(type_SEGM* segm_ptr)
{
	uint8_t data_build_error = 0;
	for (uint8_t i = 0; i < segm_ptr->expected_segm_vol; i++){
		if (segm_ptr->frame_status[i] & SEGM_FRAME_VALID){
			//
		}
		else{
			data_build_error = 1;
			break;
		}
	}
	if (data_build_error){
		return 0;
	}
	//
	segm_ptr->data_len = 0;
	for (uint8_t i = 0; i < segm_ptr->expected_segm_vol; i++){
		memcpy(	(uint8_t*)&segm_ptr->data[i*SEGM_FRAME_DATA_LEN],
				(uint8_t*)&segm_ptr->frame_array[i].field.data,
				segm_ptr->frame_array[i].field.data_len);
		segm_ptr->data_len += segm_ptr->frame_array[i].field.data_len;
	}
	//
	if (segm_ptr->new_data_ready == 0){
		segm_ptr->new_data_ready = 1;
		segm_ptr->data_cnter += 1;
	}
	segm_ptr->data_ready = 1;
	segm_ptr->frame_cnter++;
	//
	return 1;
}

/**
 * @brief запрос новых данных от блока сбора сегментированных данных
 * 
 * @param segm_ptr 
 * @param data указатель на массив, куда буду сложены данные
 * @return uint8_t кол-во данных, 0 - новых данных нет
 */
uint8_t segm_get_new_data(type_SEGM* segm_ptr, uint8_t* data)
{
	if (segm_ptr->new_data_ready){
		segm_ptr->new_data_ready = 0;
		memcpy(data, segm_ptr->data, segm_ptr->data_len);
		return segm_ptr->data_len;
	}
	else{
		return 0;
	}
}

/**
 * @brief запрос новых данных от блока сбора сегментированных данных
 * 
 * @param segm_ptr 
 * @param data указатель на массив, куда буду сложены данные
 * @return uint8_t кол-во данных, 0 - новых данных нет
 */
uint8_t segm_check_new_data(type_SEGM* segm_ptr)
{
	if (segm_ptr->new_data_ready){
		segm_ptr->new_data_ready = 0;
		return segm_ptr->data_len;
	}
	else{
		return 0;
	}
}

/**
 * @brief запрос данных от блока сбора сегментированных данных при наличии
 * 
 * @param segm_ptr 
 * @param data указатель на массив, куда буду сложены данные
 * @return uint8_t кол-во данных, 0 - новых данных нет
 */
uint8_t segm_get_data(type_SEGM* segm_ptr, uint8_t* data)
{
	if (segm_ptr->data_ready){
		memcpy(data, segm_ptr->data, segm_ptr->data_len);
		return segm_ptr->data_len;
	}
	else{
		return 0;
	}
}
