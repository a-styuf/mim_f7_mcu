
#include "mim.h"

//*** LM ***//
void mim_init(type_MIM_DEVICE* mim_ptr, uint8_t self_id, char* ver, type_PWR_CONTROL* pwr_ptr, uint8_t fabr_num, uint8_t dev_id, type_RADIO *radio_ptr, type_EXPERIMENT_CORE *ex_ptr)
{
	mim_ptr->self_id = self_id;
	mim_ptr->pwr_ptr = pwr_ptr;
	mim_ptr->fabr_num = fabr_num;
	mim_ptr->dev_id = dev_id;
	mim_ptr->radio_array = radio_ptr;
	mim_ptr->ex_ptr = ex_ptr;
	//
	mim_ctrl_init(mim_ptr);
	//
	printf("%s: MIM init\n", now());
}

/**
  * @brief  инициализация состояния управляющих параметров МС
  * @retval статус успешности инициализации: кол-во успешно инициализированных блоков
  */
int8_t mim_ctrl_init(type_MIM_DEVICE* mim_ptr)
{
	int8_t ret_val = 0;
	// инициализируем параметры управляющей структуры
	mim_ptr->ctrl.global_time_s = 0;
	mim_ptr->ctrl.mode = MIM_MODE_TRANSCEIVER;
	memset((uint8_t*)mim_ptr->pl_status, 0x00, sizeof(mim_ptr->pl_status));
	memset((uint8_t*)mim_ptr->pl_report, 0x00, sizeof(mim_ptr->pl_report));
	memset((uint8_t*)&mim_ptr->rx_fifo, 0x00, sizeof(type_MIM_RX_Fifo));
	mim_ptr->ctrl.inhibit = 0;
	mim_intervals_init(mim_ptr);
	mim_ptr->need_to_update_report = 0;
	mim_ptr->need_to_update_status = 0;
	mim_state_reset(mim_ptr);
	return ret_val;
}


/**
  * @brief  инициализация состояния управляющих параметров МС
  * @retval статус успешности инициализации: кол-во успешно инициализированных блоков
  */
int8_t mim_state_reset(type_MIM_DEVICE* mim_ptr)
{
	int8_t ret_val = 0;
	// инициализируем параметры управляющей структуры
	mim_ptr->ctrl.rst_cnt = 0;
	mim_ptr->tx_total_cnt = 0;
	mim_rx_fifo_init(mim_ptr);
	mim_status_collector(mim_ptr, MIM_STATUS_NO_ONE, MIM_STATUS_TYPE_RESET);
	mim_error_collector(mim_ptr, MIM_ERROR_NO_ONE, MIM_ERROR_TYPE_RESET);
	segm_reset_state(&mim_ptr->tr_lvl[0].segm);
	segm_reset_state(&mim_ptr->tr_lvl[1].segm);
	return ret_val;
}

/**
	* @brief  процесс обработки состояния Интерфейсного Модуля
	* @param  mim_ptr указатель на структуру управления
	* @param  time_us время МК us
  */
int8_t mim_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	uint8_t retval=0;
	uint8_t pl_num = 0;
	uint8_t num = 0;
	uint8_t module_num = 0;
	uint8_t status = 0;
	// uint8_t frame[128] = {0};
	type_MIM_DEVICE* mim_ptr = (type_MIM_DEVICE*)ctrl_struct;
	type_MIM_TP_SHARED_MEM *shared_mem = (type_MIM_TP_SHARED_MEM*)interface->shared_mem;
	//
	if ((time_us - mim_ptr->last_call_time_us) > (MIM_HANDLER_INTERVAL_MS*1000)) {
		mim_ptr->call_interval_us = time_us - mim_ptr->last_call_time_us;
		mim_ptr->last_call_time_us = time_us;
		// USER_CODE_BEGIN

		// USER_CODE_END
		retval = 1;
	}
	// обработка интервалов работы БЭ
	if (mim_interval_processor(mim_ptr, MIM_INTERVAL_1S, time_us)) {
		// USER_CODE_BEGIN
		mim_report_create(mim_ptr);
		// проверка режима работы
		mim_status_collector(mim_ptr, MIM_STATUS_OPERATION, MIM_STATUS_TYPE_SET);
		if (mim_ptr->pwr_ptr->ch_status){
			mim_error_collector(mim_ptr, MIM_ERROR_PWR, MIM_ERROR_TYPE_SET);
		}
		mim_status_collector(mim_ptr, MIM_STATUS_OPERATION, MIM_STATUS_TYPE_SET);
		// Заполнение телеметрии
		for (num = 0; num<1; num++){ //data_map - это указатель на структуру данных, одинаковый для обоих модулей, второй проход необязателен
			fill_short_tmi(mim_ptr);
			mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[num].data_map, MIM_INF_ID_SHORT_TMI_RD, (uint8_t*)&mim_ptr->sh_tmi, sizeof(mim_ptr->sh_tmi));
			//
			fill_tmi(mim_ptr);
			mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[num].data_map, MIM_INF_ID_TMI_RD, (uint8_t*)&mim_ptr->tmi, sizeof(mim_ptr->tmi));
			//
			fill_sys_tmi(mim_ptr);
			mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[num].data_map, MIM_INF_ID_READ_SYSTEM_INFO, (uint8_t*)&mim_ptr->sys_tmi, sizeof(mim_ptr->sys_tmi));
		}
		// USER_CODE_END
		retval = 1;
	}
	//
	if (mim_interval_processor(mim_ptr, MIM_INTERVAL_0_5, time_us)) {
		// user code begin
		
		// user code end
		retval = 1;
	}
	//
	if (mim_interval_processor(mim_ptr, MIM_INTERVAL_0_1, time_us)) {
		// user code begin
		mim_ptr->ctrl.global_time_s = clock_get_time_s();
		if(mim_ptr->rx_fifo.rec_num) mim_status_collector(mim_ptr, MIM_STATUS_OPERATION, MIM_STATUS_TYPE_SET);
		// user code end
		retval = 1;
	}
	//
	if (mim_interval_processor(mim_ptr, MIM_INTERVAL_EXPERIMENT, time_us)) {
		// user code begin
		if(mim_ptr->ctrl.mode == MIM_MODE_EXPERIMENT){
			mim_status_collector(mim_ptr, MIM_STATUS_EXPERIMENT, MIM_ERROR_TYPE_SET);
			switch (mim_ptr->ex_ptr->state){
				case EX_IDLE:
					ex_set_idle(mim_ptr->ex_ptr);
					mim_ptr->ex_ptr->state = EX_PREPARATION;
					break;
				case EX_PREPARATION:
					ex_set_parameters(mim_ptr->ex_ptr, 	mim_ptr->ex_ptr->shadow_param.step_num_max,
														mim_ptr->ex_ptr->shadow_param.period_s,
														mim_ptr->ex_ptr->shadow_param.data_len,
														mim_ptr->ex_ptr->shadow_param.start_nand_block,
														mim_ptr->ex_ptr->shadow_param.frame_cnter_to_save);
					mim_set_interval_value(mim_ptr, MIM_INTERVAL_EXPERIMENT, mim_ptr->ex_ptr->shadow_param.period_s);
					ex_preparation_step(mim_ptr->ex_ptr);
					mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[0].data_map, MIM_INF_ID_READ_EXP_TMI, (uint8_t*)&mim_ptr->ex_ptr->report_frame, sizeof(mim_ptr->ex_ptr->report_frame));
					mim_ptr->ex_ptr->state = EX_TX_WORK;
					break;
				case EX_TX_WORK:
					module_num = ex_tx_work_state_step(mim_ptr->ex_ptr);
					if ((module_num >= 1) && (module_num < MIM_MODULE_NUM)){
						status = radio_write_tx_fifo(&mim_ptr->radio_array[module_num - 1], &mim_ptr->ex_ptr->tx_frame);
						mim_ptr->ex_ptr->tx_frame_cnter++;
						if (status > 0) { mim_ptr->tx_total_cnt++; }
						else if (status <= 0) { mim_ptr->tx_lost_cnt++; }
					}
					else{
						//
					}
					mim_ptr->ex_ptr->state = EX_RX_WORK;
					break;
				case EX_RX_WORK:
					if(ex_rx_work_state_step(mim_ptr->ex_ptr)){
						mim_ptr->ex_ptr->state = EX_TX_WORK;
					}
					else{
						mim_ptr->ex_ptr->state = EX_REPORT;
					}
					mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[0].data_map, MIM_INF_ID_READ_EXP_TMI, (uint8_t*)&mim_ptr->ex_ptr->report_frame, sizeof(mim_ptr->ex_ptr->report_frame));
					break;
				case EX_REPORT:
					ex_report_step(mim_ptr->ex_ptr);
					mem_tr_inf_receive_id_data(mim_ptr->tr_lvl[0].data_map, MIM_INF_ID_READ_EXP_TMI, (uint8_t*)&mim_ptr->ex_ptr->report_frame, sizeof(mim_ptr->ex_ptr->report_frame));
					mim_ptr->ex_ptr->state = EX_FINISH;
					break;
				case EX_FINISH:
					mim_ptr->ctrl.mode = MIM_MODE_TRANSCEIVER;
					mim_set_interval_value(mim_ptr, MIM_INTERVAL_EXPERIMENT, 1.0);
					mim_ptr->ex_ptr->state = EX_IDLE;
					break;
			}
		}
		else {
			mim_ptr->ex_ptr->state = EX_IDLE;
			mim_status_collector(mim_ptr, MIM_STATUS_EXPERIMENT, MIM_ERROR_TYPE_RESET);
		}
		// user code end
		retval = 1;
	}
	//
	for(pl_num = MIM; pl_num < MIM_MODULE_NUM; pl_num++){
		if(interface->event[pl_num] & MIM_TP_EVENT_UPDATE_STATUS){
			interface->event[pl_num] &= ~MIM_TP_EVENT_UPDATE_STATUS;
			mim_ptr->pl_status[pl_num] = *(uint16_t*)&interface->shared_mem[512*pl_num+16];
		}
		if(interface->event[pl_num] & MIM_TP_EVENT_UPDATE_REPORT){
			interface->event[pl_num] &= ~MIM_TP_EVENT_UPDATE_REPORT;
			memcpy((uint8_t*)mim_ptr->pl_report[pl_num], (uint8_t*)&interface->shared_mem[512*pl_num+18], 16);
		}		
	}
	// вычитывание принятых покетов из радиомодулей
	for (uint8_t radio = 1; radio < MIM_MODULE_NUM; radio++){
		if(interface->event[radio] & MIM_TP_EVENT_UPDATE_LAST_FRAME){
			interface->event[radio] &= ~MIM_TP_EVENT_UPDATE_LAST_FRAME;
			mim_ptr->rx_fifo.buffer.fl.packet_type = 0x14;
			mim_ptr->rx_fifo.buffer.fl.packet_size = 306;
			memcpy((uint8_t *)&mim_ptr->rx_fifo.buffer.fl.packet, (uint8_t*)&interface->shared_mem[radio*512+128], sizeof(typePktRx));
			if (mim_write_rx_fifo_and_save_exp_data(mim_ptr, &mim_ptr->rx_fifo.buffer) == 0) {
				mim_error_collector(mim_ptr, MIM_ERROR_RX_FIFO, MIM_ERROR_TYPE_SET);
			}
		}
	}
	//
	if (mim_ptr->need_to_update_report){
		mim_ptr->need_to_update_report = 0;
		memcpy((uint8_t*)shared_mem->field.pl_report, (uint8_t*)&mim_ptr->report, 16);
		interface->event[mim_ptr->self_id] |= MIM_TP_EVENT_UPDATE_REPORT;
	}
	//
	return retval;
}

/**
  * @brief  создание отчета по работе МС
  */
void mim_report_create(type_MIM_DEVICE* mim_ptr)
{
	memset((uint8_t*)&mim_ptr->report, 0xFE, sizeof(type_MIM_REPORT));
	// инициализируем параметры управляющей структуры
	mim_ptr->report.self_id 		= mim_ptr->self_id;
	mim_ptr->report.status 			= mim_ptr->ctrl.status;
	mim_ptr->report.err			 	= mim_ptr->ctrl.err;
	mim_ptr->report.err_cnt 		= mim_ptr->ctrl.err_cnt;
	mim_ptr->report.voltage 		= 0xFEFE;
	mim_ptr->report.current 		= 0xFEFE;
	mim_ptr->report.ver[0] 			= mim_ptr->ver[0];
	mim_ptr->report.ver[1] 			= mim_ptr->ver[1];
	mim_ptr->report.ver[2] 			= mim_ptr->ver[2];
	//
	mim_ptr->need_to_update_report = 1;
}

/**
	* @brief  инициализация состояния МС
	* @param  mim_ptr: указатель на структуру управления МС
	*/
void mim_reset_state(type_MIM_DEVICE* mim_ptr)
{
	// инициализация LM
	mim_ctrl_init(mim_ptr);
}

// Управление интервалами

/**
 * @brief инициализация рабочих интервалов для модуля сопряжения
 * 
 * @param mim_ptr 
 */
void mim_intervals_init(type_MIM_DEVICE* mim_ptr)
{
	uint8_t i=0;
	float interv_default_values[MIM_INTERV_NUMBER] = DEFAULT_MIM_INTERV_VALUES_S;
	float interv_start_time[MIM_INTERV_NUMBER] = DEFAULT_MIM_DEFAULT_START_TIME_S;
	//
	for (i=0; i<MIM_INTERV_NUMBER; i++){
		mim_ptr->ctrl.intervals_us[i] = (uint64_t)(floor(interv_default_values[i] * 1000000));
		mim_ptr->ctrl.last_call_interval_times_us[i] = 0;
		mim_ptr->ctrl.first_call_time_us[i] = (uint64_t)(floor(interv_start_time[i] * 1000000));
		mim_ptr->ctrl.first_call_status[i] = 0;
	}
}

/**
 * @brief обработчик достижения отсчета интервалов
 * 
 * @param mim_ptr указатель на переменную управления
 * @param interval_id уникальный номер обрабатываемого интервала
 * @param time_us текущее время в мкс
 * @return int8_t: 1 - интервал отсчитался, 0 - интервал не досчитался
 */
int8_t mim_interval_processor(type_MIM_DEVICE* mim_ptr, uint8_t interval_id, uint64_t time_us)
{
	uint64_t interval_us;
	uint64_t last_call_interval = (time_us - mim_ptr->ctrl.last_call_interval_times_us[interval_id]);
	//
	if (mim_ptr->ctrl.first_call_status[interval_id] == 0){
		interval_us = mim_ptr->ctrl.first_call_time_us[interval_id];
	}
	else{
		interval_us = mim_ptr->ctrl.intervals_us[interval_id];
	}

	if (last_call_interval > interval_us) {
		mim_ptr->ctrl.last_call_interval_times_us[interval_id] = time_us;
		mim_ptr->ctrl.first_call_status[interval_id] = 1;
		return 1;
	}
	return 0;
}

// Управление настройками работы ЦМ

/**
  * @brief  установка измерительных интервалов
	* @param  mim_ptr указатель на структуру управления
	* @param  interval_number номер интервала для установки
	* @param  interval_value_s значение интервала для установки (5 - 7200 секунд)
  */
void mim_set_interval_value(type_MIM_DEVICE* mim_ptr, uint16_t interval_number, uint16_t interval_value_s)
{
	if (interval_number < MIM_INTERV_NUMBER){
		mim_ptr->ctrl.intervals_us[interval_number] = get_val_from_bound(interval_value_s, 0.5, 7200) * 1000000;
	}
	else{
		return;
	}
}

/**
 * @brief Инициализация fifo-приема
 * 
 * @param mim_ptr 
 * @return int8_t 
 */
int8_t mim_rx_fifo_init(type_MIM_DEVICE* mim_ptr)
{
	memset((uint8_t*)&mim_ptr->rx_fifo, 0x00, sizeof(type_MIM_RX_Fifo));
	return 0;
}

/**
 * @brief запись данных в fifo для принятых данных
 * @param mim_ptr 
 * @param rx_frame_ptr 
 * @return int8_t 1 - ОК, 0 - записано, но с потерей пакета из-за переполнения
 */
int8_t mim_write_rx_fifo(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr)
{
	uint8_t ret_val = 1;
	memcpy((uint8_t*)&mim_ptr->rx_fifo.array[mim_ptr->rx_fifo.wr_ptr], (uint8_t *)&rx_frame_ptr->array[0], sizeof(type_MIM_INF_ID_RX_DATA));
	mim_ptr->rx_fifo.rec_full++;
	//
	if((++mim_ptr->rx_fifo.wr_ptr) >= MIM_RX_FRAME_FIFO_SIZE) mim_ptr->rx_fifo.wr_ptr = 0;
	//
	if ((mim_ptr->rx_fifo.wr_ptr == mim_ptr->rx_fifo.rd_ptr)){
		if(++mim_ptr->rx_fifo.rd_ptr >= MIM_RX_FRAME_FIFO_SIZE) mim_ptr->rx_fifo.rd_ptr = 0;
		mim_ptr->rx_fifo.rec_lost++;
		ret_val = 0;
	}
	//
	mim_rx_fifo_rec_num_update(mim_ptr);
	return ret_val;
}

/**
 * @brief запись данных в fifo для принятых данных и сохранение параметров для эксперимента
 * @param mim_ptr 
 * @param rx_frame_ptr 
 * @return int8_t 1 - ОК, 0 - записано, но с потерей пакета из-за переполнения
 */
int8_t mim_write_rx_fifo_and_save_exp_data(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr)
{
	uint8_t status = 0;
	status = mim_write_rx_fifo(mim_ptr, rx_frame_ptr);
	// анализ данных для эксперимента
	ex_analyze_rx_frame(mim_ptr->ex_ptr, *rx_frame_ptr);
	return status;
}

/**
 * @brief чтение данных из fifo для принятых данных
 * 
 * @param mim_ptr 
 * @param rx_frame_ptr 
 * @return int8_t 1 - ОК, 0 - нет данных
 */
int8_t mim_read_rx_fifo(type_MIM_DEVICE* mim_ptr, type_MIM_INF_ID_RX_DATA* rx_frame_ptr)
{
	if(mim_ptr->rx_fifo.rec_num){
		memcpy((uint8_t *)&rx_frame_ptr->array[0], (uint8_t*)&mim_ptr->rx_fifo.array[mim_ptr->rx_fifo.rd_ptr], sizeof(type_MIM_INF_ID_RX_DATA));
		if(++mim_ptr->rx_fifo.rd_ptr >= MIM_RX_FRAME_FIFO_SIZE) mim_ptr->rx_fifo.rd_ptr = 0;
		mim_rx_fifo_rec_num_update(mim_ptr);
		return 1;
	}
	else{
		return 0;
	}
}

/**
 * @brief чтение данных из fifo в массив из 1014 байт для передачи в МФР
 * 
 * @param mim_ptr 
 * @param rx_data буфер для передачи 1014 байт 
 */
void mim_read_rx_fifo_and_form_mrf_packet(type_MIM_DEVICE* mim_ptr, uint8_t* rx_data)
{
	type_MIM_INF_ID_RX_DATA rx_packet;
	uint16_t max_len = 1014;
	uint16_t max_wr_ptr = (1014 - sizeof(type_MIM_INF_ID_RX_DATA)); // такая длина для того, что бы не забирать лишний пакет из FIFO, который может не влезть
	uint16_t wr_ptr = 0;
	while (wr_ptr < max_wr_ptr){
		if (mim_read_rx_fifo(mim_ptr, &rx_packet)){
            rx_packet.fl.packet_size = 4 + rx_packet.fl.packet.tot_size;
			if(rx_packet.fl.packet_size < (max_len - wr_ptr)){
				memcpy((uint8_t*)&rx_data[wr_ptr], rx_packet.tr_lvl.array, rx_packet.fl.packet_size);
				wr_ptr += rx_packet.fl.packet_size;
			}
			else{
				break;
			}
		}
		else{
			break;
		}
	}
	memset((uint8_t*)&rx_data[wr_ptr], 0xFE, max_len - wr_ptr);
}

/**
 * @brief обновление количества записей в FIFO по состояниям указателя чтения и записи.
 * 
 * @param mim_ptr 
 */
void mim_rx_fifo_rec_num_update(type_MIM_DEVICE* mim_ptr)
{
	mim_ptr->rx_fifo.rec_num = (mim_ptr->rx_fifo.wr_ptr >= mim_ptr->rx_fifo.rd_ptr) ? (mim_ptr->rx_fifo.wr_ptr - mim_ptr->rx_fifo.rd_ptr) 
																					: (MIM_RX_FRAME_FIFO_SIZE - (mim_ptr->rx_fifo.rd_ptr - mim_ptr->rx_fifo.wr_ptr));
	mim_ptr->rx_fifo.rec_num_max = (mim_ptr->rx_fifo.rec_num > mim_ptr->rx_fifo.rec_num_max) ? mim_ptr->rx_fifo.rec_num : mim_ptr->rx_fifo.rec_num_max;
}


//*** заполнение тми и маяка ***//
/**
  * @brief  заполнение маяка и ТМИ
  * @param  mim_ptr: структура управления для МС
  */
void fill_short_tmi(type_MIM_DEVICE* mim_ptr)
{
	type_MIM_Short_TMI sh_tmi;
	// short TMI
	sh_tmi.fields.label = 0xF1;
	sh_tmi.fields.mim_status = mim_ptr->ctrl.status;
	sh_tmi.fields.err_status = mim_ptr->ctrl.err;
	sh_tmi.fields.rx_fifo_cnt = mim_ptr->rx_fifo.rec_num;
	sh_tmi.fields.rx_total_cnt = mim_ptr->rx_fifo.rec_full;
	sh_tmi.fields.tx_total_cnt = mim_ptr->tx_total_cnt;
	sh_tmi.fields.crc8 = crc8ccitt(sh_tmi.array, sizeof(type_MIM_Short_TMI) - 1, 0xF1);
	memcpy(mim_ptr->sh_tmi.array, sh_tmi.array, sizeof(type_MIM_Short_TMI));
}

/**
  * @brief  заполнение ТМИ
  * @param  mim_ptr: структура управления для МС
  */
void fill_tmi(type_MIM_DEVICE* mim_ptr)
{
	type_MIM_TMI tmi;
	frame_create_header((uint8_t*)&tmi.fields.header, mim_ptr->fabr_num, mim_ptr->dev_id, 0x00, 0x00, MIM_FR_TYPE_TMI, mim_ptr->frame_num++, NULL);
	//
	tmi.fields.mim_status = mim_ptr->ctrl.status; 			// +12
	tmi.fields.err_status = mim_ptr->ctrl.err;	 			// +14
	tmi.fields.error_cnt = mim_ptr->ctrl.err_cnt;			// +16
	//
	tmi.fields.rx_fifo_cnt =  mim_ptr->rx_fifo.rec_num;		// +18
	tmi.fields.rx_total_cnt = mim_ptr->rx_fifo.rec_full;	// +20
	tmi.fields.tx_total_cnt = mim_ptr->tx_total_cnt; 		// +22
	tmi.fields.rx_fifo_lost_frames = mim_ptr->rx_fifo.rec_lost;			// +26
	tmi.fields.tx_fifo_lost_frames = mim_ptr->tx_lost_cnt;				// +30
	//
	pwr_get_tmi_report(mim_ptr->pwr_ptr, &tmi.fields.pwr_tmi_report);	// +34
	//
	tmi.fields.segm1_err_cnter = mim_ptr->tr_lvl[0].segm.err_cnter;	
	tmi.fields.segm1_status = mim_ptr->tr_lvl[0].segm.status;		
	tmi.fields.segm1_data_cnter = mim_ptr->tr_lvl[0].segm.data_cnter;		
	tmi.fields.segm1_trans_cnter = mim_ptr->tr_lvl[0].segm.frame_cnter;		
	tmi.fields.segm1_data_len = mim_ptr->tr_lvl[0].segm.data_len;		
	//
	tmi.fields.segm2_err_cnter = mim_ptr->tr_lvl[1].segm.err_cnter;	
	tmi.fields.segm2_status = mim_ptr->tr_lvl[1].segm.status;		
	tmi.fields.segm2_data_cnter = mim_ptr->tr_lvl[1].segm.data_cnter;		
	tmi.fields.segm2_trans_cnter = mim_ptr->tr_lvl[1].segm.frame_cnter;		
	tmi.fields.segm2_data_len = mim_ptr->tr_lvl[1].segm.data_len;	
	//
	tmi.fields.nand_id = mim_ptr->ex_ptr->nand_ptr->id;
	//
	memset(tmi.fields.reserve, 0xFE, sizeof(tmi.fields.reserve));		//+96
	frame_crc16_calc(tmi.array);
	memcpy(mim_ptr->tmi.array, tmi.array, sizeof(type_MIM_TMI));
}

/**
  * @brief  заполнение ТМИ
  * @param  mim_ptr: структура управления для МС
  */
void fill_sys_tmi(type_MIM_DEVICE* mim_ptr)
{
	type_MIM_SYS_TMI sys_tmi;
	int i = 0;
	frame_create_header((uint8_t*)&sys_tmi.fl.p1.header, mim_ptr->fabr_num, mim_ptr->dev_id, 0x00, 0x00, MIM_FR_TYPE_SYS_TMI_PART_1, mim_ptr->frame_num++, NULL);
	frame_create_header((uint8_t*)&sys_tmi.fl.p2.header, mim_ptr->fabr_num, mim_ptr->dev_id, 0x00, 0x00, MIM_FR_TYPE_SYS_TMI_PART_2, mim_ptr->frame_num++, NULL);
	//
	for(i=0; i<MIM_MODULE_NUM; i++){
		if (i < 7){
			memcpy((uint8_t*)&sys_tmi.fl.p1.report[i], (uint8_t*)&mim_ptr->pl_report[i], 16);
		}
		else{
			memcpy((uint8_t*)&sys_tmi.fl.p2.report[i-7], (uint8_t*)&mim_ptr->pl_report[i], 16);
		}
	}
	//
	memset(sys_tmi.fl.p1.reserve, 0xFE, sizeof(sys_tmi.fl.p1.reserve));
	memset(sys_tmi.fl.p2.reserve, 0xFE, sizeof(sys_tmi.fl.p2.reserve));
	//
	frame_crc16_calc(sys_tmi.array[0]);
	frame_crc16_calc(sys_tmi.array[1]);
	//
	memcpy(mim_ptr->sys_tmi.array[0], sys_tmi.array[0], sizeof(typeSingleFrameStruct));
	memcpy(mim_ptr->sys_tmi.array[1], sys_tmi.array[1], sizeof(typeSingleFrameStruct));
}

/**
 * @brief обработка статусов
 * 
 * @param mim_ptr 
 * @param flag флаг статуса, сброс статуса при использовании NO_ONE статуса с типом RESET
 * @param type тип работы со статусом - установка или очистка 
 */
void mim_status_collector(type_MIM_DEVICE* mim_ptr, uint16_t flag, uint8_t type)
{
	switch(type){
		case MIM_STATUS_TYPE_SET:
				mim_ptr->ctrl.status |= flag;
			break;
		case MIM_STATUS_TYPE_RESET:
			if(flag == MIM_STATUS_NO_ONE){
				mim_ptr->ctrl.status = 0;
			}
			else{
				mim_ptr->ctrl.status &= ~(flag);
			}
			break;
	}
}

/**
 * @brief обработка ошибок
 * 
 * @param mim_ptr 
 * @param flag флаг ошибки, сброс ошибок и счетчика ошибок при использовании NO_ONE статуса с типом RESET
 * @param type тип работы со статусом - установка или очистка 
 */
void  mim_error_collector(type_MIM_DEVICE* mim_ptr, uint16_t flag, uint8_t type)
{
	switch(type){
		case MIM_ERROR_TYPE_SET:
			mim_ptr->ctrl.err |= flag;
			mim_ptr->ctrl.err_cnt++;
			break;
		case MIM_ERROR_TYPE_RESET:
			if(flag == MIM_STATUS_NO_ONE){
				mim_ptr->ctrl.err = 0;
				mim_ptr->ctrl.err_cnt = 0;
			}
			else{
				mim_ptr->ctrl.err &= ~(flag);
			}
			break;
	}
}


//*** функции общего назначения

/**
 * @brief если число внутри границ - используется оно, если нет, то ближайшая граница
 * 
 * @param val 
 * @param min 
 * @param max 
 * @return uint16_t 
 */
uint16_t get_val_from_bound(uint16_t val, uint16_t min, uint16_t max)
{
	if (val > max) return max;
	else if (val < min) return min;
	return val;
}

/**
 * @brief если число внутри границ (включительно) - возвращает 1, иначе 0
 * 
 * @param val 
 * @param min 
 * @param max 
 * @return uint16_t 
 */
uint16_t check_val_in_bound(uint16_t val, uint16_t min, uint16_t max)
{
	if ((val <= max) && (val >= min)) return 1;
	else return 0;
}

uint32_t get_uint32_val_from_bound(uint32_t val, uint32_t min, uint32_t max) //если число внутри границ - используется оно, если нет, то ближайшая граница
{
	if (val > max) return max;
	else if (val < min) return min;
	return val;
}
