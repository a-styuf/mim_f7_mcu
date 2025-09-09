/**
 * @file radio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-10-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "radio.h"

/**
 * @brief 
 * 
 * @param radio_ptr 
 * @param pwr_ch 
 * @param mb_addr 
 * @param huart_req 
 * @param self_id 
 */
void radio_init(type_RADIO* radio_ptr, uint16_t self_id, type_PWR_CHANNEL* pwr_ch_ptr, uint16_t can_addr)
{
	// интерфейсы к аппаратной части
	radio_ptr->pwr_ch_ptr = pwr_ch_ptr;
	// общие настройки
	radio_ptr->self_id = self_id;
	radio_ptr->can_addr = can_addr;
	//
	radio_status_collector(radio_ptr, RADIO_STATUS_NO_ONE, RADIO_STATUS_RELEASE);
	// инициализация рабочих интервалов
	radio_intervals_init(radio_ptr);
	//
	memset((uint8_t*)&radio_ptr->report, 0xFE, sizeof(type_RADIO_Report));
	memset((uint8_t*)&radio_ptr->tx_ctrl, 0x00, sizeof(type_RADIO_TX_CTRL));
	memset((uint8_t*)&radio_ptr->rx_ctrl, 0x00, sizeof(type_RADIO_RX_CTRL));
	//
	radio_ptr->last_call_time_us = 0;
	radio_ptr->call_interval_us = 0;
}

/**
	* @brief  функция для запуска в планировщике задач обработки светодиодов
	* @param  ctrl_struct указатель на программную модель устройства
	* @param  time_us глобальное время
	* @param  interface interface из процесса на верх
  */
int8_t radio_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	int8_t retval = 0;
	type_RADIO* radio_ptr = (type_RADIO*)ctrl_struct;

	type_RADIO_TP_SHARED_MEM *shared_mem = (type_RADIO_TP_SHARED_MEM*)interface->shared_mem;
	//
	HAL_GPIO_WritePin(GPIOC, 1<<14, GPIO_PIN_SET);
	//
	if ((time_us - radio_ptr->last_call_time_us) > (RADIO_PROCESS_PERIOD_MS*1000)) {
		radio_ptr->call_interval_us = time_us - radio_ptr->last_call_time_us;
		radio_ptr->last_call_time_us = time_us;
		//
		radio_status_collector(radio_ptr, RADIO_STATUS_WORK, RADIO_STATUS_SET);
		radio_status_collector(radio_ptr, RADIO_PWR_ERR_MASK & (radio_ptr->pwr_ch_ptr->error << RADIO_PWR_ERR_SHIFT), RADIO_STATUS_SET);
		radio_create_report(radio_ptr);
		//передача статуса через интерфейс планировщика задач
		shared_mem->field.status = radio_ptr->status;
		interface->event[radio_ptr->self_id] |= RADIO_TP_EVENT_UPDATE_STATUS;
		//
	}
	// обработка интервалов работы БЭ
	if (radio_interval_processor(radio_ptr, RADIO_POLLING_INTERVAL, time_us)) {
		// USER_CODE_BEGIN
		if ((interface->event[radio_ptr->self_id] & RADIO_TP_EVENT_UPDATE_CMD_STATUS_REPORT) == 0)
		{
			radio_tx_get_cmd_status(radio_ptr);
			radio_rx_get_cmd_status(radio_ptr);
			radio_generate_cmd_status_report(radio_ptr);
			shared_mem->field.rm_cs_report = radio_ptr->cmd_status_report.word;
			interface->event[radio_ptr->self_id] |= RADIO_TP_EVENT_UPDATE_CMD_STATUS_REPORT;
		}
		// USER_CODE_END
		retval = 1;
	}
	if (radio_interval_processor(radio_ptr, RADIO_TX_INTERVAL, time_us)) {
		// USER_CODE_BEGIN
		radio_ptr->tx_ctrl.fifo_ena = 1;
		if (radio_read_tx_fifo(radio_ptr, &radio_ptr->tx_ctrl.IVarTxSet_wr)){
			radio_tx_data(radio_ptr, CMD_TX_SETPARAMS_TRANSMIT);
		}
		// USER_CODE_END
		retval = 1;
	}
	if (radio_interval_processor(radio_ptr, RADIO_RX_INTERVAL, time_us)) {
		// USER_CODE_BEGIN
		radio_ptr->rx_ctrl.fifo_ena = 1;
		//
		radio_rx_get_rx_info(radio_ptr);
		if (radio_rx_check_info(radio_ptr)){
			radio_rx_get_packet(radio_ptr);
		}
		// USER_CODE_END
		retval = 1;
	}
	//
	if (radio_ptr->need_to_update_report){
		radio_ptr->need_to_update_report = 0;
		memcpy((uint8_t*)shared_mem->field.pl_report, (uint8_t*)&radio_ptr->report, 16);
		interface->event[radio_ptr->self_id] |= RADIO_TP_EVENT_UPDATE_REPORT;
	}
	// передача прочитанного кадра в модуль верхнего
	if (radio_read_rx_fifo(radio_ptr, &shared_mem->field.packet)){
		interface->event[radio_ptr->self_id] |= RADIO_TP_EVENT_UPDATE_LAST_FRAME;
	}
	//
	HAL_GPIO_WritePin(GPIOC, 1<<14, GPIO_PIN_RESET);
	//
	return retval;
}

/**
 * @brief Создание системного отчета по работе ПН
 * 
 * @param radio_ptr 
 */
void radio_create_report(type_RADIO* radio_ptr) //TODO: дозаполнить отчет
{
	memset((uint8_t*)&radio_ptr->report, 0xFE, sizeof(type_RADIO_Report));
	//
	radio_ptr->report.field.self_id 	= radio_ptr->self_id;
	radio_ptr->report.field.reserve 	= 0xFE;
	radio_ptr->report.field.error_cnt 	= (radio_ptr->error_cnter + radio_ptr->pwr_ch_ptr->error_cnt) & 0xFF;
	radio_ptr->report.field.status 		= (radio_ptr->status);
	radio_ptr->report.field.u_5v0 		= (uint16_t)(radio_ptr->pwr_ch_ptr->u_5v_self*256);
	radio_ptr->report.field.u_3V3 		= (uint16_t)(radio_ptr->pwr_ch_ptr->u_3v3_self*256);
	radio_ptr->report.field.current 	= (uint16_t)(radio_ptr->pwr_ch_ptr->i_5v_pwr*256);
	radio_ptr->report.field.rx_cnt 		= radio_ptr->tx_ctrl.frame_cnter;
	radio_ptr->report.field.tx_cnt 		= radio_ptr->rx_ctrl.frame_cnter;
	//
	radio_ptr->need_to_update_report = 1;
}

//*** функции общения с Радио ***//
/**
 * @brief Проверка статуса
 * 
 * @param radio_ptr 
 */
int8_t radio_check_status(type_RADIO* radio_ptr)
{	
	int error = 0;
	volatile typeIdxMask idx;
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 3, 0, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxCtrl_rd, sizeof(typeIVarTxCtrl));
	if (error >= 0) {
		return 0;
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
 * @brief Запрос пакета с данными
 * 
 * @param radio_ptr 
 */
void radio_get_packet(type_RADIO* radio_ptr)
{	
	#ifdef DEBUG
	printf("%s: radio %d: get packet\n", now(), radio_ptr->self_id);
	#endif
}

/**
 * @brief Запрос настроек модуля
 * 
 * @param radio_ptr 
 */
void radio_get_settings(type_RADIO* radio_ptr)
{	
	#ifdef DEBUG
	printf("%s: radio %d: get settings\n", now(), radio_ptr->self_id);
	#endif
}

// Управление интервалами

/**
 * @brief инициализация рабочих интервалов
 * 
 * @param radio_ptr 
 */
void radio_intervals_init(type_RADIO* radio_ptr)
{
	uint8_t i=0;
	float interv_default_values[RADIO_INTERV_NUMBER] = DEFAULT_RADIO_INTERV_VALUES_S;
	float interv_start_time[RADIO_INTERV_NUMBER] = DEFAULT_RADIO_DEFAULT_START_TIME_S;
	//
	for (i=0; i<RADIO_INTERV_NUMBER; i++){
		radio_ptr->intervals[i] = interv_default_values[i];
		radio_ptr->last_call_interval_times[i] = 0;
		radio_ptr->first_call_status[i] = 0;
		radio_ptr->first_call_time[i] = interv_start_time[i];
	}
}

/**
 * @brief обработчик достижения отсчета интервалов
 * 
 * @param radio_ptr указатель на переменную управления
 * @param interval_id уникальный номер обрабатываемого интервала
 * @param time_us текущее время в мкс
 * @return int8_t: 1 - интервал отсчитался, 0 - интервал не досчитался
 */
int8_t radio_interval_processor(type_RADIO* radio_ptr, uint8_t interval_id, uint64_t time_us)
{
	uint64_t interval_us;
	uint64_t last_call_interval = (time_us - radio_ptr->last_call_interval_times[interval_id]);
	//
	if (radio_ptr->first_call_status[interval_id] == 0){
		interval_us = radio_ptr->first_call_time[interval_id] * 1000000;
	}
	else{
		interval_us = radio_ptr->intervals[interval_id] * 1000000;
	}

	if (last_call_interval > interval_us) {
		radio_ptr->last_call_interval_times[interval_id] = time_us;
		radio_ptr->first_call_status[interval_id] = 1;
		return 1;
	}
	return 0;
}

/**
  * @brief  установка измерительных интервалов
	* @param  radio_ptr указатель на структуру управления
	* @param  interval_number номер интервала для установки
	* @param  interval_value_s значение интервала для установки
  */
void radio_set_interval_value(type_RADIO* radio_ptr, uint16_t interval_number, uint16_t interval_value_s)
{
	float interv_min[RADIO_INTERV_NUMBER] = DEFAULT_RADIO_MIN_S;
	float interv_max[RADIO_INTERV_NUMBER] = DEFAULT_RADIO_MAX_S;
	if (interval_number < RADIO_INTERV_NUMBER){
		radio_ptr->intervals[interval_number] = radio_get_val_from_bound(interval_value_s, interv_min[interval_number], interv_max[interval_number]);
	}
	else{
		return;
	}
}

/**
 * @brief формирование пакета управления передатчиком
 * 
 * @param radio_ptr 
 * @param bw  		ISM : 6..9                   
 * @param sf  		ISM : 6..12                  
 * @param cr  		ISM : 1..4 : 4/5..4/8        
 * @param crc_  	ISM : 0 or 1                 
 * @param ldr  		ISM : 0 or 1                 
 * @param sync  	ISM : e.g.  0x12             
 * @param r_freq  	ISM : 861000000..1020000000  
 * @param f_pwr  	ISM : 0..15 : min..max
 */
void radio_form_tx_cfg(type_RADIO* radio_ptr, uint8_t bw, uint8_t sf, uint8_t cr, uint8_t crc_, uint8_t ldr, uint8_t sync, uint32_t r_freq, uint8_t f_pwr)
{
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.BW 		= bw;
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.SF 		= sf;
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.CR 		= cr;
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.CRC_ 	= crc_;
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.LDR 	= ldr;
	radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm.Sync 	= sync;
	radio_ptr->tx_ctrl.IVarTxSet_wr.RF.RFreq 		= r_freq;
	radio_ptr->tx_ctrl.IVarTxSet_wr.RF.FPwr 		= f_pwr;
}

/**
 * @brief формирования пакета данных для передачи
 * 
 * @param radio_ptr 
 * @param data 
 * @param len длина данных
 */
void radio_form_tx_data(type_RADIO* radio_ptr, uint8_t* data, uint8_t len)
{
	radio_ptr->tx_ctrl.IVarTxSet_wr.PLeng = len;
	memcpy((uint8_t*)radio_ptr->tx_ctrl.IVarTxSet_wr.Payload, data, len);
}

/**
 * @brief 
 * 
 * @param radio_ptr 
 * @param mode 
 */
void radio_tx_data(type_RADIO* radio_ptr, uint8_t mode)
{
	int error = 0;
	volatile typeIdxMask idx;
	switch(mode){
		case CMD_TX_SETPARAMS:
			idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 2, 0x00, 0);
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm, (8 + 5));
			radio_tx_send_cmd(radio_ptr, CMD_TX_SETPARAMS);
			break;
		case CMD_TX_TRANSMIT:
			idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 2, 0x00, 0);
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxSet_wr.PLeng, radio_ptr->tx_ctrl.IVarTxSet_wr.PLeng + 1);
			//
			radio_tx_send_cmd(radio_ptr, CMD_TX_TRANSMIT);
		break;
		case CMD_TX_SETPARAMS_TRANSMIT:
			idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 2, 0x00, 0);
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxSet_wr.LoraPrm, sizeof(typeIVarTxSet));
			//
			radio_tx_send_cmd(radio_ptr, CMD_TX_SETPARAMS_TRANSMIT);
			break;
		default:
		break;
	}
	//
	if (error >= 0) {
		// radio_ptr->tx_ctrl.frame_cnter += 1;
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
	}
}

/**
 * @brief отправка команды в передатчик
 * 
 * @param radio_ptr 
 * @param cmd типы команд согласно:
 *  CMD_TX_SETPARAMS - 1,
 *  CMD_TX_TRANSMIT - 2,
 *  CMD_TX_SETPARAMS_TRANSMIT - 3
 */
void radio_tx_send_cmd(type_RADIO* radio_ptr, uint8_t cmd)
{
	int error = 0;
	volatile typeIdxMask idx;
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 3, 0x00, 0);
	switch(cmd){
		case CMD_TX_SETPARAMS:
			radio_ptr->tx_ctrl.IVarTxCtrl_wr.Cmd = CMD_TX_SETPARAMS;
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxCtrl_wr, 1);
			break;
		case CMD_TX_TRANSMIT:
			radio_ptr->tx_ctrl.IVarTxCtrl_wr.Cmd = CMD_TX_TRANSMIT;
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxCtrl_wr, 1);
			break;
		case CMD_TX_SETPARAMS_TRANSMIT:
			radio_ptr->tx_ctrl.IVarTxCtrl_wr.Cmd = CMD_TX_SETPARAMS_TRANSMIT;
			error = CANVar_Write(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxCtrl_wr, 1);
			break;
		default:
			radio_status_collector(radio_ptr, RADIO_OTHER_ERROR, RADIO_STATUS_SET);
			return;
	}
	//
	if (error >= 0) {
		if ((cmd == CMD_TX_TRANSMIT) || (cmd == CMD_TX_SETPARAMS_TRANSMIT) || (cmd == CMD_TX_SETPARAMS)) {
			radio_tx_get_cmd_status(radio_ptr); // запрос параметров ддя асинхронного обновления cmd и status для радиомодуля
		}
		if ((cmd == CMD_TX_TRANSMIT) || (cmd == CMD_TX_SETPARAMS_TRANSMIT)){
			radio_ptr->tx_ctrl.frame_cnter += 1;
		}
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
	}
}

/**
 * @brief отправка тестовых данных для отладки
 * 
 * @param radio_ptr 
 */
void radio_tx_test_data(type_RADIO* radio_ptr)
{
	uint8_t test_data[64];
	// формирование тестовых данных
	for (uint8_t i = 0; i < 64; i++){
		test_data[i] = i;
	}
	test_data[0] = radio_ptr->tx_ctrl.test_frame_num++;
	//
	radio_form_tx_cfg(	radio_ptr, 
						8, 			// ISM : bw		- 6..9                   
						10,			// ISM : sf		- 6..12                  
						1, 			// ISM : cr		- 1..4 : 4/5..4/8        
						1, 			// ISM : crc_	- 0 or 1                 
						1, 			// ISM : ldr	- 0 or 1                 
						0x12, 		// ISM : sync	- e.g.  0x12             
						868000000, 	// ISM : r_freq	- 861000000..1020000000  
						0);			// ISM : f_pwr	- 0..15 : min..max
	radio_form_tx_data(radio_ptr, test_data, 64);
	radio_tx_data(radio_ptr, CMD_TX_SETPARAMS_TRANSMIT);
}

/**
 * @brief чтение настроек передатчика
 * 
 * @param radio_ptr 
 * @return int8_t 
 */
int8_t radio_tx_get_settings(type_RADIO* radio_ptr)
{
	//складываем настройки в 
	int error = 0;
	volatile typeIdxMask idx;
	//
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 2, 0x00, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxSet_rd, 16);
	//
	if (error >= 0) return 0;
	else {
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}


/**
 * @brief проверка статуса выполнения команды
 * 
 * @param radio_ptr 
 * @param cmd
 * @return int8_t 
 */
int8_t radio_tx_get_cmd_status(type_RADIO* radio_ptr)
{
	int error = 0;
	volatile typeIdxMask idx;
	//
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 3, 0x00, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->tx_ctrl.IVarTxCtrl_rd, sizeof(typeIVarTxCtrl));
	//
	if (error >= 0) {
		if(radio_ptr->tx_ctrl.IVarTxCtrl_rd.Status  == -1) {
			radio_status_collector(radio_ptr, RADIO_STATUS_TX_MODULE, RADIO_STATUS_SET);
		}
		return 0;
	}
	else {
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
  * @brief  запись посылки в TX FIFO для последующей отправки
	* @param  radio_ptr указатель на структуру управления
	* @param  tx_frame кадр для отправки
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t radio_write_tx_fifo(type_RADIO* radio_ptr, typeIVarTxSet* tx_frame_ptr)
{
	radio_status_collector(radio_ptr, RADIO_STATUS_TX_FIFO_NOT_EMPTY, RADIO_STATUS_RELEASE);
	//
	if (radio_ptr->tx_ctrl.fifo_ena){
		if (radio_ptr->tx_ctrl.fifo.rec_num >= RADIO_TX_FIFO_DEPTH){
			radio_status_collector(radio_ptr, RADIO_STATUS_TX_FIFO_FULL, RADIO_STATUS_SET);
			return -1;
		}
		else{
			memcpy((uint8_t*)&radio_ptr->tx_ctrl.fifo.array[radio_ptr->tx_ctrl.fifo.rec_num], (uint8_t *)tx_frame_ptr, sizeof(typeIVarTxSet));
			radio_ptr->tx_ctrl.fifo.rec_num += 1;
			if (radio_ptr->tx_ctrl.fifo.rec_num > radio_ptr->tx_ctrl.fifo.rec_max) radio_ptr->tx_ctrl.fifo.rec_max = radio_ptr->tx_ctrl.fifo.rec_num;  //сбор информации о заполненности fifo
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  взятие измерения из fifo работаем с массивами по 128Б
	* @param  radio_ptr указатель на структуру управления
	* @param  tx_frame кадр для отправки
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t radio_read_tx_fifo(type_RADIO* radio_ptr, typeIVarTxSet* tx_frame_ptr)
{
	if (radio_ptr->tx_ctrl.fifo.rec_num == 0){
		return 0;
	}
	else{
		radio_ptr->tx_ctrl.fifo.rec_num -= 1;
		memcpy((uint8_t*)tx_frame_ptr, &radio_ptr->tx_ctrl.fifo.array[0], sizeof(typeIVarTxSet));
		memmove(&radio_ptr->tx_ctrl.fifo.array[0], &radio_ptr->tx_ctrl.fifo.array[1], sizeof(typeIVarTxSet)*radio_ptr->tx_ctrl.fifo.rec_num);
		memset((uint8_t*)&radio_ptr->tx_ctrl.fifo.array[radio_ptr->tx_ctrl.fifo.rec_num], 0x00, sizeof(typeIVarTxSet)*(RADIO_TX_FIFO_DEPTH - radio_ptr->tx_ctrl.fifo.rec_num));
		//
		radio_status_collector(radio_ptr, RADIO_STATUS_TX_FIFO_FULL, RADIO_STATUS_RELEASE);
		return 1;
	}
}

/**
 * @brief чтение статусов принятых пакетов из памяти радиомодулей
 * 
 * @param radio_ptr 
 * @return int8_t 0 - OK, !0 - ошибка
 */
int8_t radio_rx_get_rx_info(type_RADIO* radio_ptr)
{
	int error = 0;
	volatile typeIdxMask idx;
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 6, 0, 1 );
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarRxPktInfo_rd, sizeof(typeIVarRxPktInfo));
	if (error >= 0) return 0;
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
 * @brief Проверка данных с принятой информацией для принятия решения по приему данных
 * 
 * @param radio_ptr 
 * @return int8_t 0 - нет новых пакетов в радиомодуле, 1 - необходимо запросить пакет данных
 */
int8_t radio_rx_check_info(type_RADIO* radio_ptr)
{
	if (radio_ptr->rx_ctrl.IVarRxPktInfo_rd.PktCount > 0 ){
		return 1;
	}
	return 0;
}

/**
 * @brief чтение пакета, принятого радиомодулем
 * 
 * @param radio_ptr 
 * @return int8_t 
 */
int8_t radio_rx_get_packet(type_RADIO* radio_ptr)
{
	int error = 0;
	int8_t status;
	volatile typeIdxMask idx;
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 7, 0, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarPktR_rd, sizeof(typePktRx));
	if (error >= 0) {
		#ifdef DEBUG
		// printf("%s: RADIO %d: check packet, idx = 0x%08X, ctrl = 0x%02X, stat = 0x%02X\t", now(), radio_ptr->self_id, idx.u32, radio_ptr->tx_ctrl.IVarTxCtrl_rd.Cmd, radio_ptr->tx_ctrl.IVarTxCtrl_rd.Status);
		#endif
		if (1){
			status = radio_write_rx_fifo(radio_ptr, &radio_ptr->rx_ctrl.IVarPktR_rd);
			if (status > 0) {
				radio_ptr->rx_ctrl.frame_cnter++;
			}
			else if (status == 0) {} //ожидание инициализации
			else {
				radio_status_collector(radio_ptr, RADIO_STATUS_RX_FIFO_FULL, RADIO_STATUS_SET);
			}
		}
		return 0;
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
 * @brief установка параметров работы приемников модуля
 * 
 * @param radio_ptr 
 * @param som_num 
 * @param rx_settings 
 * @return int8_t 
 */
int8_t radio_rx_set_settings(type_RADIO* radio_ptr, typeIVarRxSet* rx_settings)
{
	//складываем настройки в 
	int error = 0;
	volatile typeIdxMask idx;
	//
	memcpy((uint8_t*)&radio_ptr->rx_ctrl.IVarRxSet_wr[0], (uint8_t*)rx_settings, sizeof(typeIVarRxSet)*4);
	//
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 4, 0x00, 0);
	error = CANVar_Write(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarRxSet_wr[0], sizeof(typeIVarRxSet)*4);
	//
	if (error >= 0) {
		radio_rx_send_cmd(radio_ptr, CMD_RX_SETPARAMS);
		return 0;
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
 * @brief отправка выбранной команды в блок управления приемниками
 * 
 * @param radio_ptr 
 * @param cmd
 * @return int8_t 
 */
int8_t radio_rx_send_cmd(type_RADIO* radio_ptr, uint8_t cmd)
{
	//складываем настройки в 
	int error = 0;
	volatile typeIdxMask idx;
	
	radio_ptr->rx_ctrl.IVarRxCtrl_wr.Cmd = cmd;
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 5, 0x00, 0);
	error = CANVar_Write(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarRxCtrl_wr, 1);
	//
	if (error >= 0) {
		radio_rx_get_cmd_status(radio_ptr); // асинхронное обновление cmd и status отправки
		return 0;
	}
	else{
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
 * @brief отправка выбранной команды в блок управления приемниками
 * 
 * @param radio_ptr 
 * @param cmd
 * @return int8_t 
 */
int8_t radio_rx_get_cmd_status(type_RADIO* radio_ptr)
{
	int error = 0;
	volatile typeIdxMask idx;
	//
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 5, 0x00, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarRxCtrl_rd, sizeof(typeIVarRxCtrl));
	//
	if (error >= 0) {
		for (uint8_t mod_num = 0;  mod_num < RADIO_SOM_NUM; mod_num++){
			if(radio_ptr->rx_ctrl.IVarRxCtrl_rd.Status[mod_num]  == -1) {
				radio_status_collector(radio_ptr, (RADIO_STATUS_MODULE_0 << mod_num), RADIO_STATUS_SET);
			}
		}
		return 0;
	}
	else {
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

int8_t radio_rx_get_settings(type_RADIO* radio_ptr)
{
	//складываем настройки в 
	int error = 0;
	volatile typeIdxMask idx;
	//
	idx.u32 = generate_can_id_mask(radio_ptr->can_addr, 4, 0x00, 1);
	error = CANVar_Read(idx, (uint8_t*)&radio_ptr->rx_ctrl.IVarRxSet_rd, (4*sizeof(typeIVarRxSet)));
	//
	if (error >= 0) return 0;
	else {
		if (radio_ptr->pwr_ch_ptr->mode != PWR_CH_OFF) radio_status_collector(radio_ptr, RADIO_STATUS_CAN_ERR, RADIO_STATUS_SET);
		return error;
	}
}

/**
  * @brief  запись посылки в RX FIFO для последующей передачи по RS-485
	* @param  radio_ptr указатель на структуру управления
	* @param  rx_frame кадр для отправки
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t radio_write_rx_fifo(type_RADIO* radio_ptr, typePktRx* rx_frame_ptr)
{
	if (radio_ptr->rx_ctrl.fifo_ena){
		radio_status_collector(radio_ptr, RADIO_STATUS_RX_FIFO_NOT_EMPTY, RADIO_STATUS_RELEASE);
		if (radio_ptr->rx_ctrl.fifo.rec_num >= RADIO_RX_FIFO_DEPTH){
			radio_status_collector(radio_ptr, RADIO_STATUS_RX_FIFO_FULL, RADIO_STATUS_SET);
			return -1;
		}
		else{
			memcpy((uint8_t*)&radio_ptr->rx_ctrl.fifo.array[radio_ptr->rx_ctrl.fifo.rec_num], (uint8_t *)rx_frame_ptr, sizeof(typePktRx));
			radio_ptr->rx_ctrl.fifo.rec_num += 1;
			if (radio_ptr->rx_ctrl.fifo.rec_num > radio_ptr->rx_ctrl.fifo.rec_max) radio_ptr->rx_ctrl.fifo.rec_max = radio_ptr->rx_ctrl.fifo.rec_num;  //сбор информации о заполненности fifo
			return 1;
		}
	}
	return 0;
}

/**
  * @brief  чтение в RX FIFO для последующей передачи по RS-485
	* @param  radio_ptr указатель на структуру управления
	* @param  rx_frame кадр для отправки
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t radio_read_rx_fifo(type_RADIO* radio_ptr, typePktRx* rx_frame_ptr)
{
	if (radio_ptr->rx_ctrl.fifo.rec_num == 0){
		return 0;
	}
	else{
		radio_ptr->rx_ctrl.fifo.rec_num -= 1;
		memcpy((uint8_t*)rx_frame_ptr, &radio_ptr->rx_ctrl.fifo.array[0], sizeof(typePktRx));
		memmove(&radio_ptr->rx_ctrl.fifo.array[0], &radio_ptr->rx_ctrl.fifo.array[1], sizeof(typePktRx)*radio_ptr->rx_ctrl.fifo.rec_num);
		memset((uint8_t*)&radio_ptr->rx_ctrl.fifo.array[radio_ptr->rx_ctrl.fifo.rec_num], 0x00, sizeof(typePktRx)*(RADIO_RX_FIFO_DEPTH - radio_ptr->rx_ctrl.fifo.rec_num));
		//
		radio_status_collector(radio_ptr, RADIO_STATUS_RX_FIFO_FULL, RADIO_STATUS_RELEASE);
		return 1;
	}
}

/**
 * @brief Сборка отчета по состояниям команд, выполняемых радиомодулем, и их статусов
 * 
 * @param radio_ptr 
 * @return uint32_t отчет
 */
uint32_t radio_generate_cmd_status_report(type_RADIO* radio_ptr)
{	
	radio_ptr->cmd_status_report.fd.tx_cmd = radio_ptr->tx_ctrl.IVarTxCtrl_rd.Cmd & 0x0F;
	radio_ptr->cmd_status_report.fd.tx_status = (-1*radio_ptr->tx_ctrl.IVarTxCtrl_rd.Status) & 0x0F;
	//
	radio_ptr->cmd_status_report.fd.rx_cmd = radio_ptr->rx_ctrl.IVarRxCtrl_rd.Cmd & 0x0F;
	radio_ptr->cmd_status_report.fd.rx_status_0 = (-1*radio_ptr->rx_ctrl.IVarRxCtrl_rd.Status[0]) & 0x0F;
	radio_ptr->cmd_status_report.fd.rx_status_1 = (-1*radio_ptr->rx_ctrl.IVarRxCtrl_rd.Status[1]) & 0x0F;
	radio_ptr->cmd_status_report.fd.rx_status_2 = (-1*radio_ptr->rx_ctrl.IVarRxCtrl_rd.Status[2]) & 0x0F;
	radio_ptr->cmd_status_report.fd.rx_status_3 = (-1*radio_ptr->rx_ctrl.IVarRxCtrl_rd.Status[3]) & 0x0F;
	//
	return radio_ptr->cmd_status_report.word;
}


///*** функции внутреннего назначения ***///

/**
 * @brief обработка статусов полезной нагрузки
 * 
 * @param radio_ptr 
 * @param flag флаг статуса, при использовании типа  NO_ONE с флагом CLEAR очищает статус и счетчик
 * @param type 
 */
void  radio_status_collector(type_RADIO* radio_ptr, uint16_t flag, uint8_t type)
{
	switch(type){
		case RADIO_STATUS_SET:
			radio_ptr->status |= flag;
			// для некоторых типов статусов/ошибок используем счетчик
			// собираем по отдельным if для возможности множественного изменения по флагу через ИЛИ
			if(flag & RADIO_STATUS_CAN_ERR) 	radio_ptr->error_cnter += 1;
			if(flag & RADIO_STATUS_MODULE_0) 	radio_ptr->error_cnter += 1;
			if(flag & RADIO_STATUS_MODULE_1) 	radio_ptr->error_cnter += 1;
			if(flag & RADIO_STATUS_MODULE_2) 	radio_ptr->error_cnter += 1;
			if(flag & RADIO_STATUS_MODULE_3) 	radio_ptr->error_cnter += 1;
			if(flag & RADIO_OTHER_ERROR) 		radio_ptr->error_cnter += 1;
			break;
		case RADIO_STATUS_RELEASE:
			radio_ptr->status &= ~(flag);
			if  (flag == RADIO_STATUS_NO_ONE) { radio_ptr->status = 0x00; radio_ptr->error_cnter = 0x00; };
			break;
	}
}

/**
 * @brief обработка статусов полезной нагрузки
 * 
 * @param radio_ptr 
 * @param flag флаг статуса, при использовании типа  NO_ONE с флагом CLEAR очищает статус и счетчик
 * @param type 
 */
void  radio_status_reset(type_RADIO* radio_ptr)
{
	radio_status_collector(radio_ptr, RADIO_STATUS_NO_ONE, RADIO_STATUS_RELEASE);
	pwr_ch_error_collector(radio_ptr->pwr_ch_ptr, PWR_CH_ERR_NO_ERR, PWR_CH_STATUS_SET);
}

/**
 * @brief если число внутри границ - используется оно, если нет, то ближайшая граница
 * 
 * @param val 
 * @param min 
 * @param max 
 * @return uint16_t 
 */
uint16_t radio_get_val_from_bound(uint16_t val, uint16_t min, uint16_t max)
{
	if (val > max) return max;
	else if (val < min) return min;
	return val;

}
/**
 * @brief Генерация id_msk в формате typeIdxMask
 * 
 * @param dev_id 
 * @param var_id 
 * @param offset 
 * @param rtr 1 - чтение, 0 - запись 
 */
uint32_t generate_can_id_mask(uint32_t dev_id, uint32_t var_id, uint32_t offset, uint32_t rtr)
{
	volatile typeIdxMask id;
	*((uint32_t*)&id) = 0;
	//
	id.uf.res1 = 0x00;
	id.uf.RTR = rtr;
	id.uf.res2 = 0x00;
	id.uf.Offset = offset;
	id.uf.VarId = var_id;
	id.uf.DevId = dev_id;
	//
	return id.u32;
}
