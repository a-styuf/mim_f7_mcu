/**
 * @file experiment.c
 * @author Alexey Styuf (a-styuf@yandex.m)
 * @brief 
 * @version 0.1
 * @date 2024-09-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "experiment.h"

void ex_init(type_EXPERIMENT_CORE *ex_ptr, type_NAND *nand_ptr, uint8_t self_id, uint8_t fabr_num, uint8_t dev_id)
{
  memset((uint8_t*)ex_ptr, 0x00, sizeof(type_EXPERIMENT_CORE));
  //
  ex_ptr->self_id = self_id;
  ex_ptr->fabr_num = fabr_num;
  ex_ptr->dev_id = dev_id;
	//
  ex_reset(ex_ptr);
  ex_ptr->nand_ptr = nand_ptr;
  //
}

void ex_reset(type_EXPERIMENT_CORE *ex_ptr)
{
  type_EX_TX_Param tx_param;
  //
  ex_ptr->state = EX_IDLE;
	ex_status_collector(ex_ptr, EX_STATUS_NO_ONE, EX_STATUS_RELEASE);
	ex_ptr->error_cnt = 0;
	ex_ptr->period_s = EX_DEFAULT_PERIOD_S;
	ex_ptr->step_num = 0;
  ex_ptr->step_num_max = EX_DEFAULT_STEP_NUM;
  ex_ptr->frame_num = 0;
  ex_ptr->start_nand_block = 0;
  ex_ptr->page_num = 0;
  ex_ptr->rd_data_finish_flag = 0;
  ex_ptr->tx_frame_cnter = 0;
	ex_ptr->rx_frame_cnter = 0;
  ex_ptr->rx_saved_frame_cnter = 0;
  ex_ptr->start_nand_block = 0;
	ex_ptr->page_num = 0;
	memset((uint8_t*)&ex_ptr->last_tx_msg, 0x00, sizeof(type_EX_TX_Param));
	memset((uint8_t*)&ex_ptr->tx_msg_buffer, 0x00, sizeof(type_RADIO_EX_TX_MSG_BUFFER));
  //
  tx_param = _ex_generate_tx_param(ex_ptr, 1, 7, 10, 1, 1, 0, 0x12, 0x0, 0x00, 861000000, 0, 0, 16);
  ex_set_tx_msg(ex_ptr, 0, tx_param.array);
}

void ex_set_idle(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_status_collector(ex_ptr, EX_STATUS_NO_ONE, EX_STATUS_RELEASE);
	ex_ptr->step_num = 0;
  ex_ptr->tx_msg_buffer.buffer_msg_ptr = 0;
  ex_ptr->tx_frame_cnter = 0;
	ex_ptr->rx_frame_cnter = 0;
  ex_ptr->rx_saved_frame_cnter = 0;
  ex_ptr->frame_num = 0;
  ex_ptr->start_nand_block = 0;
  ex_ptr->page_num = 0;
  ex_ptr->rd_data_finish_flag = 0;
  ex_ptr->error_cnt = 0;
	memset((uint8_t*)&ex_ptr->last_tx_msg, 0x00, sizeof(type_EX_TX_Param));
  ex_rx_fifo_init(ex_ptr);
}

void ex_preparation_step(type_EXPERIMENT_CORE *ex_ptr)
{
  nand_lin_write_block_set(ex_ptr->nand_ptr, ex_ptr->start_nand_block);
  ex_status_collector(ex_ptr, EX_STATUS_WORK, EX_STATUS_SET);
  ex_ptr->ex_seq_num++;
  ex_generate_report_frames(ex_ptr, 0);
}

uint8_t ex_tx_work_state_step(type_EXPERIMENT_CORE *ex_ptr)
{
  uint8_t module_num;
  module_num = ex_generate_tx_frame(ex_ptr);
  return module_num;
}

int8_t ex_rx_work_state_step(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_generate_rx_data_frames(ex_ptr);
  ex_generate_report_frames(ex_ptr, 0);
  ex_ptr->step_num++;
  if (ex_ptr->step_num >= ex_ptr->step_num_max) return 0;
  else return 1;
}

int8_t ex_report_step(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_generate_report_frames_and_save_to_nand(ex_ptr);
  nand_lin_write_finish(ex_ptr->nand_ptr);
  return 0;
}

int8_t ex_set_parameters(type_EXPERIMENT_CORE *ex_ptr, uint16_t step_num, uint16_t period_s, uint8_t data_len, uint32_t start_nand_block, uint8_t frame_to_save)
{
  ex_ptr->period_s = period_s;
  ex_ptr->step_num_max = MIN(step_num, EX_MAX_STEP_NUM);
  ex_ptr->data_len = 0xFE; // такого параметра пака нет
  ex_ptr->start_nand_block = MIN(start_nand_block, (ex_ptr->nand_ptr->hnand->Config.BlockNbr - 1));
  ex_ptr->frame_cnter_to_save = MAX(1, MIN(frame_to_save, EX_RX_FRAME_CNTER_TO_SAVE_MAX));
  return 0;
}

int8_t ex_get_current_parameters(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_ptr->current_param.period_s = ex_ptr->period_s;
  ex_ptr->current_param.step_num_max = ex_ptr->step_num_max;
  ex_ptr->current_param.data_len = ex_ptr->data_len;
  ex_ptr->current_param.start_nand_block = ex_ptr->start_nand_block;
  ex_ptr->current_param.frame_cnter_to_save = ex_ptr->frame_cnter_to_save;
  return 0;
}

type_EX_TX_Param _ex_generate_tx_param(type_EXPERIMENT_CORE *ex_ptr, uint8_t module_num, uint8_t BW, uint8_t SF, uint8_t CR, 
                                                                uint8_t CRC_, uint8_t LDR, uint8_t Sync, uint8_t InvertIQ, uint8_t Preleng,
                                                                uint32_t RFreq, uint8_t FPwr, uint8_t  PwrNoOn, uint8_t PLeng)
{
  type_EX_TX_Param tx_param;
  //
  memset(tx_param.array, 0x00, sizeof(type_EX_TX_Param));
  // генерация настроек
  tx_param.f.module_num = module_num;
  tx_param.f.BW = BW;
  tx_param.f.SF = SF;
  tx_param.f.CR = CR;
  tx_param.f.CRC_ = CRC_;
  tx_param.f.LDR = LDR;
  tx_param.f.Sync = Sync;
  tx_param.f.InvertIQ = InvertIQ;
  tx_param.f.Preleng = Preleng;
  tx_param.f.RFreq = RFreq;
  tx_param.f.FPwr = FPwr;
  tx_param.f.PwrNoOn = PwrNoOn;
  tx_param.f.PLeng = PLeng;
  return tx_param;
}

int8_t ex_set_tx_msg(type_EXPERIMENT_CORE *ex_ptr, uint8_t msg_num, uint8_t *tx_parameters)
{
  if (msg_num < EX_MSG_BUFFER_LENG){
    memcpy((uint8_t*)ex_ptr->tx_msg_buffer.msg[msg_num].array, tx_parameters, sizeof(type_EX_TX_Param));
		return 0;
  }
	return -1;
}

int8_t ex_start(type_EXPERIMENT_CORE *ex_ptr)
{
  if (ex_ptr->state == EX_IDLE){
    ex_ptr->state = EX_PREPARATION;
    return 1;
  }
  else{
    ex_status_collector(ex_ptr, EX_STATUS_LAUNCH_ERR, EX_STATUS_SET);
    return 0;
  }
}

int8_t ex_stop(type_EXPERIMENT_CORE *ex_ptr)
{
    if (ex_ptr->state != EX_IDLE){
    ex_ptr->state = EX_IDLE;
  ex_set_idle(ex_ptr);
    return 1;
  }
  else{
    ex_status_collector(ex_ptr, EX_STATUS_LAUNCH_ERR, EX_STATUS_SET);
    return 0;
  }
}

/**
 * @brief формирование пакета для передачи данных эксперимента в поле структуры управления экспериментом (tx_frame)
 * @param ex_ptr указать структуры управления эксперимента
 * @return int8_t 0 - нет необходимости передавать, 1-13 - номер модуля для передачи
 */
int8_t ex_generate_tx_frame(type_EXPERIMENT_CORE *ex_ptr)
{
  typeIVarTxSet tx_f;
  uint8_t module_num = 0;
  uint8_t i;
  uint8_t msg_num;
  //
	msg_num = ex_ptr->tx_msg_buffer.buffer_msg_ptr;
	//
  for(i=0; i<EX_MSG_BUFFER_LENG; i++) {
    ex_ptr->tx_msg_buffer.buffer_msg_ptr++;
    if (ex_ptr->tx_msg_buffer.buffer_msg_ptr >= EX_MSG_BUFFER_LENG) ex_ptr->tx_msg_buffer.buffer_msg_ptr = 0;
    //
    if (ex_ptr->tx_msg_buffer.msg[ex_ptr->tx_msg_buffer.buffer_msg_ptr].f.module_num == 0){
			//
		}
    else{
      break;
    }
  }
  if (i==EX_MSG_BUFFER_LENG) ex_status_collector(ex_ptr, EX_STATUS_TX_EMPTY, EX_STATUS_SET);
  //
  ex_ptr->last_tx_msg = ex_ptr->tx_msg_buffer.msg[msg_num];
  module_num = ex_ptr->last_tx_msg.f.module_num;
  //
  tx_f.LoraPrm.BW = ex_ptr->tx_msg_buffer.msg[msg_num].f.BW;
  tx_f.LoraPrm.SF = ex_ptr->tx_msg_buffer.msg[msg_num].f.SF;
  tx_f.LoraPrm.CR = ex_ptr->tx_msg_buffer.msg[msg_num].f.CR;
  tx_f.LoraPrm.CRC_ = ex_ptr->tx_msg_buffer.msg[msg_num].f.CRC_;
  tx_f.LoraPrm.LDR = ex_ptr->tx_msg_buffer.msg[msg_num].f.LDR;
  tx_f.LoraPrm.Sync = ex_ptr->tx_msg_buffer.msg[msg_num].f.Sync;
	tx_f.LoraPrm.InvertIQ = ex_ptr->tx_msg_buffer.msg[msg_num].f.InvertIQ;
  tx_f.LoraPrm.Preleng = ex_ptr->tx_msg_buffer.msg[msg_num].f.Preleng;
  tx_f.RF.RFreq = ex_ptr->tx_msg_buffer.msg[msg_num].f.RFreq;
  tx_f.RF.FPwr = ex_ptr->tx_msg_buffer.msg[msg_num].f.FPwr;
	tx_f.PwrNoOn = ex_ptr->tx_msg_buffer.msg[msg_num].f.PwrNoOn;
  memset(tx_f.res2, 0x00, sizeof(tx_f.res2));
  //
  
  //
  tx_f.PLeng = MAX(ex_ptr->tx_msg_buffer.msg[msg_num].f.PLeng, 4);
  ex_ptr->data_len = tx_f.PLeng;
  //
  memset(tx_f.Payload, 0xFE, tx_f.PLeng);
  *(uint32_t*)tx_f.Payload = ex_generate_uniq_id(ex_ptr);
  //
  memcpy((uint8_t*)&ex_ptr->tx_frame, (uint8_t*)&tx_f, sizeof(typeIVarTxSet));
  //
  return module_num;
}

uint32_t ex_generate_uniq_id(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_ptr->current_uniq_id = ((ex_ptr->step_num & 0xFFFF) << 16) | ((ex_ptr->tx_msg_buffer.buffer_msg_ptr & 0xFF) << 8) | ((ex_ptr->ex_seq_num & 0xFF) << 0);
  return ex_ptr->current_uniq_id;
}

int8_t ex_analyze_rx_frame(type_EXPERIMENT_CORE *ex_ptr, type_MIM_INF_ID_RX_DATA rx_frame)
{
  ex_generate_uniq_id(ex_ptr);
  if (*(uint32_t*)rx_frame.fl.packet.payload == ex_ptr->current_uniq_id){
    ex_ptr->last_rx_data.dev_id  = rx_frame.fl.packet.dev_id;
    ex_ptr->last_rx_data.modem_id  = rx_frame.fl.packet.modem_id;
    ex_ptr->last_rx_data.if_chain = rx_frame.fl.packet.if_chain;
    memset((uint8_t*)ex_ptr->last_rx_data.reserve, 0x00, sizeof(ex_ptr->last_rx_data.reserve));
    ex_ptr->last_rx_data.count_ms  = rx_frame.fl.packet.count_ms;
    ex_ptr->last_rx_data.rssi_channel  = rx_frame.fl.packet.rssic;
    ex_ptr->last_rx_data.rssi_signal = rx_frame.fl.packet.rssis;
    ex_ptr->last_rx_data.snr = rx_frame.fl.packet.snr;
    ex_ptr->last_rx_data.snr_max = rx_frame.fl.packet.snr_max;
    ex_ptr->last_rx_data.snr_min = rx_frame.fl.packet.snr_min;
    //
    ex_rx_fifo_write(ex_ptr, &ex_ptr->last_rx_data);
    ex_ptr->rx_frame_cnter++;
    return 1;
  }
  else{
    ex_status_collector(ex_ptr, EX_STATUS_RX_ERROR, EX_STATUS_SET);
    return 0;
  }
}

int8_t ex_generate_rx_data_frames(type_EXPERIMENT_CORE *ex_ptr)
{
  uint8_t time_array[8];
  uint8_t frame_cnter = 0;
  type_EX_RX_Data frame;

  ex_ptr->rd_data_finish_flag = 0;

  while((ex_ptr->rx_data_fifo.rec_num != 0) || (ex_ptr->rd_data_finish_flag == 0)){
    frame_create_header((uint8_t*)&frame.fields.header, ex_ptr->fabr_num, ex_ptr->dev_id, 0x00, 0x00, EX_FRAME_RX_DATA, ex_ptr->frame_num++, NULL);
    //
    clock_get_bcd_array_with_fractional_part(time_array);
    frame.fields.time_fractional_ms = (RTC_Bcd2ToByte(time_array[6])*256) + (RTC_Bcd2ToByte(time_array[7]));
		frame.fields.uniq_tx_id = ex_generate_uniq_id(ex_ptr);
    frame.fields.reserve_1 = 0x00;
    //
		memcpy(frame.fields.tx_parameters, (uint8_t*)ex_ptr->last_tx_msg.array, sizeof(type_EX_TX_Param));
    //
		memcpy(frame.fields.position_data, ex_ptr->position_data, sizeof(frame.fields.position_data));
    //
    for (uint8_t i=0; i<2; i++){
      if(ex_rx_fifo_read(ex_ptr, &ex_ptr->last_rx_data)){
        frame.fields.rx_data[i] = ex_ptr->last_rx_data;
        ex_ptr->rx_saved_frame_cnter++;
      }
      else{
        memset((uint8_t*)&frame.fields.rx_data[i], 0xFE, sizeof(type_RADIO_EX_RX_DATA));
        ex_ptr->rd_data_finish_flag = 1;
      }
    }
    //
    memset(frame.fields.reserve_2, 0xFE, sizeof(frame.fields.reserve_2));
    frame_crc16_calc(frame.array);
    memcpy(ex_ptr->rx_frame.array, frame.array, sizeof(type_EX_RX_Data));
    // Блок записи в FIFO
    nand_lin_write_frame(ex_ptr->nand_ptr, ex_ptr->rx_frame.array);
    frame_cnter++;
    //
    if (ex_ptr->rd_data_finish_flag) return 1;
    else if (frame_cnter >= ex_ptr->frame_cnter_to_save) {
      ex_ptr->rd_data_finish_flag = 1;
      ex_status_collector(ex_ptr, EX_STATUS_FRAME_CNTER_EXCEED, EX_STATUS_SET);
      return 1;
    }
  }
  return 0;
}

int8_t ex_generate_report_frames(type_EXPERIMENT_CORE *ex_ptr, uint16_t frame_num)
{
  type_EX_REPORT_FRAME frame;
  frame_create_header((uint8_t*)&frame.fields.header, ex_ptr->fabr_num, ex_ptr->dev_id, 0x00, 0x00, EX_FRAME_REPORT, 0, NULL);
  //
  frame.fields.status = ex_ptr->status;				
  frame.fields.state = ex_ptr->state;				  
  frame.fields.error_cnt = ex_ptr->error_cnt;			
  frame.fields.step_num_max = ex_ptr->step_num_max;			
  frame.fields.step_num = ex_ptr->step_num;
  frame.fields.period_s = ex_ptr->period_s;			
  frame.fields.data_len = ex_ptr->data_len;			
  frame.fields.frame_cnter_to_save = ex_ptr->frame_cnter_to_save;
  frame.fields.start_nand_block = ex_ptr->start_nand_block;
  frame.fields.wr_nand_frame_cnter = ex_ptr->nand_ptr->wr_frame_cnt;
  frame.fields.line_read_error = ex_ptr->nand_ptr->line_read_error;
  frame.fields.line_write_error = ex_ptr->nand_ptr->line_write_error;
  ex_generate_uniq_id(ex_ptr);
  frame.fields.uniq_tx_id = ex_ptr->current_uniq_id;
  frame.fields.tx_frame_cnter = ex_ptr->tx_frame_cnter;
  frame.fields.rx_frame_cnter = ex_ptr->rx_frame_cnter;
  frame.fields.rx_saved_frame_cnter = ex_ptr->rx_saved_frame_cnter;
  //
  memset(frame.fields.reserve, 0xFE, sizeof(frame.fields.reserve));		//+96
  frame_crc16_calc(frame.array);
  memcpy(ex_ptr->report_frame.array, frame.array, sizeof(type_EX_REPORT_FRAME));
  return 1;
}

int8_t ex_generate_report_frames_and_save_to_nand(type_EXPERIMENT_CORE *ex_ptr)
{
  ex_generate_report_frames(ex_ptr, ex_ptr->frame_num++);
  //
  nand_lin_write_frame(ex_ptr->nand_ptr, ex_ptr->report_frame.array);
  return 1;
}


int8_t ex_rx_fifo_init(type_EXPERIMENT_CORE *ex_ptr)
{
  memset((uint8_t*)&ex_ptr->rx_data_fifo, 0x00, sizeof(type_EX_RX_Data_Fifo));
  return 0;
}

/**
  * @brief  запись посылки в RX DATA FIFO
	* @param  ex_ptr указатель на структуру управления
	* @param  rx_data указатель на данные
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t ex_rx_fifo_write(type_EXPERIMENT_CORE *ex_ptr, type_RADIO_EX_RX_DATA* rx_data)
{
  if (ex_ptr->rx_data_fifo.rec_num >= EX_RX_DATA_FIFO_DEPTH){
    ex_status_collector(ex_ptr, EX_STATUS_RX_FIFO_FULL, EX_STATUS_SET);
    return -1;
  }
  else{
    memcpy((uint8_t*)&ex_ptr->rx_data_fifo.array[ex_ptr->rx_data_fifo.rec_num], (uint8_t *)rx_data, sizeof(type_RADIO_EX_RX_DATA));
    ex_ptr->rx_data_fifo.rec_num += 1;
    if (ex_ptr->rx_data_fifo.rec_num > ex_ptr->rx_data_fifo.rec_max) ex_ptr->rx_data_fifo.rec_max = ex_ptr->rx_data_fifo.rec_num;  //сбор информации о заполненности fifo
    return 1;
  }
}

/**
  * @brief  чтение RX DATA FIFO
	* @param  ex_ptr указатель на структуру управления
	* @param  rx_frame кадр для отправки
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t ex_rx_fifo_read(type_EXPERIMENT_CORE *ex_ptr, type_RADIO_EX_RX_DATA* rx_data)
{
	if (ex_ptr->rx_data_fifo.rec_num == 0){
		return 0;
	}
	else{
		ex_ptr->rx_data_fifo.rec_num -= 1;
		memcpy((uint8_t*)rx_data, &ex_ptr->rx_data_fifo.array[0], sizeof(type_RADIO_EX_RX_DATA));
		memmove(&ex_ptr->rx_data_fifo.array[0], &ex_ptr->rx_data_fifo.array[1], sizeof(type_RADIO_EX_RX_DATA)*ex_ptr->rx_data_fifo.rec_num);
		memset((uint8_t*)&ex_ptr->rx_data_fifo.array[ex_ptr->rx_data_fifo.rec_num], 0x00, sizeof(type_RADIO_EX_RX_DATA)*(EX_RX_DATA_FIFO_DEPTH - ex_ptr->rx_data_fifo.rec_num));
		//
		return 1;
	}
}

void ex_status_collector(type_EXPERIMENT_CORE *ex_ptr, uint16_t flag, uint8_t type)
{
	switch(type){
		case EX_STATUS_SET:
			ex_ptr->status |= flag;
			// для некоторых типов статусов/ошибок используем счетчик
			// собираем по отдельным if для возможности множественного изменения по флагу через ИЛИ
			if(flag & EX_STATUS_ERR) 	ex_ptr->error_cnt += 1;
			if(flag & EX_STATUS_TX_EMPTY) 	ex_ptr->error_cnt += 1;
			if(flag & EX_STATUS_RX_ERROR) 	ex_ptr->error_cnt += 1;
			if(flag & EX_STATUS_LAUNCH_ERR) 	ex_ptr->error_cnt += 1;
			break;
		case EX_STATUS_RELEASE:
			ex_ptr->status &= ~(flag);
			if  (flag == EX_STATUS_NO_ONE) { ex_ptr->status = 0x00; ex_ptr->error_cnt = 0x00; };
			break;
	}
}
