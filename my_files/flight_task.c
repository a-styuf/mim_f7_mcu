/**
 * @file flight_task.c
 * @author a-styuf (a-styuf@yandex.ru)
 * @brief полетные задания для работы со стандартными циклограммами планировщика задач
 * @version 0.1
 * @date 2023-03-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "flight_task.h"

/**
 * @brief инициализация полетного задания
 * 
 * @param ft_ptr 
 * @return int8_t 1 - успешная инициализация, 0 - ошибка
 */
int8_t ft_init(typeFT *ft_ptr, uint8_t self_num, uint8_t sat_id, uint8_t dev_id)
{
    //изначальное обнуление параметров
    memset((uint8_t *)ft_ptr, 0x00, sizeof(typeFT));
    //
    ft_ptr->self_num = self_num;
    ft_ptr->sat_id = sat_id;
    ft_ptr->dev_id = dev_id;
    return 1;
}

/**
	* @brief  функция для запуска в планировщике задач обработки светодиодов
	* @param  ctrl_struct указатель на программную модель устройства
	* @param  time_us глобальное время
	* @param  interface interface из процесса на верх
  */
int8_t ft_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface)
{
	int8_t retval = 0;
	typeFT* ft_ptr = (typeFT*)ctrl_struct;
	ft_process(ft_ptr, time_us/1000);
	return retval;
}

/**
 * @brief обработка шага полетного задания
 * 
 * @param ft_ptr 
 * @param time_ms 
 * @return int8_t 
 */
int8_t ft_process(typeFT *ft_ptr, uint32_t time_ms)
{   
    switch(ft_ptr->mode){
        case FT_MODE_OFF:
            //
            break;
        case FT_MODE_START:
                ft_set_mode(ft_ptr, FT_MODE_WORK);
                ft_ptr->ctrl.step_delay = 0; //для перехода к новому шагу
                ft_ptr->ctrl.rpt_value = 0; //для перехода к новому шагу
                ft_ptr->ctrl.last_step_time = time_ms;
                ft_refresh_go_to_parameters(ft_ptr);
            break;
        case FT_MODE_WORK:
            ft_mode_work_process(ft_ptr, time_ms);
            break;
        case FT_MODE_PAUSE:
            ft_ptr->ctrl.pause_time += time_ms - ft_ptr->ctrl.last_pause_time;
            ft_ptr->ctrl.last_pause_time = time_ms;
            break;
    }
    return ft_ptr->mode;
}

/**
 * @brief Установка режима работы полетного задания с обработкой корректности управления
 * 
 * @param ft_ptr указатель на структуру управления полетным заданием
 * @param mode режим работы полетного задания
 * @return int8_t -1 - некорректный режим, 0 - не удалось установить режим, так как это невозможно, 1 - режим успешно установлен
 */
int8_t ft_set_mode(typeFT *ft_ptr, uint8_t mode)
{
    printf("%s: ft_<%d>_mode_set:", now(), ft_ptr->self_num);
    switch(mode){
        case FT_MODE_OFF:
            __ft_step_cast_reset(ft_ptr);
            ft_ptr->mode = mode;
            printf("<off>\n");
            return 1;
        case FT_MODE_WORK:
            if(ft_ptr->mode == FT_MODE_WORK) {}
            else if(ft_ptr->mode == FT_MODE_START) {
                ft_ptr->mode = FT_MODE_WORK;
                printf("<work>\n");
                return 1;
            }
            else if(ft_ptr->mode == FT_MODE_PAUSE){
                ft_ptr->ctrl.last_step_time += ft_ptr->ctrl.pause_time;
                ft_ptr->ctrl.pause_time = 0;
                ft_ptr->mode = FT_MODE_WORK;
                printf("<work>\n");
                return 1;
            }
            break;
        case FT_MODE_START:
            ft_ptr->mode = FT_MODE_START;
            printf("<start>\n");
            return 1;
        case FT_MODE_PAUSE:
            if (ft_ptr->mode == FT_MODE_WORK){
                ft_ptr->ctrl.pause_time = 0;
                ft_ptr->mode = FT_MODE_PAUSE;
                printf("<pause>\n");
                return 1;
            }
            else {}
            break;
        default:
            printf("<error>\n");
            return -1;
    }
    printf("<skip>\n");
    return 0;
}

void ft_mode_work_process(typeFT *ft_ptr, uint32_t time_ms)
{   
    uint8_t step_num = 0;
    if ((time_ms - ft_ptr->ctrl.last_step_time) >= ft_ptr->ctrl.step_delay) {  // достижение условия окончания паузы до следующего вызова
        ft_ptr->ctrl.last_step_time = time_ms;
        // обработка повторных вызовов шага
        if (ft_ptr->ctrl.rpt_value == 0) {
            ft_create_step_cast(ft_ptr);
            if (ft_ptr->ctrl.go_to_cnt[ft_ptr->ctrl.step_num] == 0){
                ft_ptr->ctrl.step_num++;
                // сборка информации о количествах переходов goto
                for(step_num=0; step_num < ft_ptr->ctrl.step_num; step_num++){
                    ft_ptr->ctrl.go_to_cnt[step_num] = ft_ptr->ctrl.go_to_max[step_num];
                }
            }
            else{
                ft_ptr->ctrl.go_to_cnt[ft_ptr->ctrl.step_num]--;
                ft_ptr->ctrl.step_num = ft_ptr->ctrl.step_cast.fields.go_to;
            }
        }
        else {
            ft_ptr->ctrl.rpt_value--;
        }
        ft_func_run(ft_ptr);
        //
        if (ft_ptr->ctrl.step_num > FT_LEN_STEP){
            ft_set_mode(ft_ptr, FT_MODE_OFF);
        }
    }
    else {
        return;
    }
}

/**
 * @brief создание слепка шага полетного задания по номеру шага
 * 
 * @param ft_ptr 
 * @return int8_t 1 - слепок создан, 0 - слепок не может быть создан
 */
int8_t ft_create_step_cast(typeFT *ft_ptr)
{
    memset((uint8_t*)&ft_ptr->ctrl.step_cast, 0x00, sizeof(typeFTStep));
    //
    if (ft_ptr->ctrl.step_num < FT_LEN_STEP) {
        ft_ptr->ctrl.step_cast = ft_ptr->task.step[ft_ptr->ctrl.step_num];
        ft_ptr->ctrl.rpt_value = ft_ptr->ctrl.step_cast.fields.rpt_cnt;
        ft_ptr->ctrl.step_delay = ft_ptr->ctrl.step_cast.fields.delay_ms;
        if(ft_check_step_cast_crc(ft_ptr) == 0) return 0;
        return 1;
    }
    else{
        ft_ptr->ctrl.rpt_value = 0;
        ft_ptr->ctrl.step_delay = 0;
        return 0;
    }
}

/**
 * @brief функция запуска функции полетного задания
 * 
 * @param ft_ptr 
 * @return int32_t 
 */
int32_t ft_func_run(typeFT* ft_ptr)
{
    uint32_t type, cmd;
    // подготовка переменных
    type = ft_ptr->ctrl.step_cast.fields.type;
    cmd = ft_ptr->ctrl.step_cast.fields.cmd;
    memset(ft_ptr->ctrl.data_to_save, 0xFE, sizeof(ft_ptr->ctrl.data_to_save)); 
    ft_ptr->ctrl.data_len = 0;
    ft_ptr->ctrl.func_ret_val = 0;
    // запуск функции
    if (ft_ptr->ctrl.step_cast.fields.label == FT_MARK){
        if (ft_ptr->f_group[type].func[cmd].func != NULL){
            ft_ptr->ctrl.func_ret_val = ft_ptr->f_group[type].func[cmd].func(   ft_ptr->f_group[type].func[cmd].ctrl_struct, 
                                                                                ft_ptr->ctrl.step_cast.fields.data,
                                                                                ft_ptr->ctrl.data_to_save, 
                                                                                &ft_ptr->ctrl.data_len);
            ft_function_retval_check(ft_ptr);
            // сохранение данных
            if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_SAVE_DATA_ENA){
                switch(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_SAVE_DATA_TYPE){
                    case 0:  //! формируем кадр из сырых данных
                        frame_create_header((uint8_t*)&ft_ptr->ctrl.ft_frame.header,
                                            ft_ptr->sat_id, ft_ptr->dev_id, 0x01, SINGLE_FRAME_TYPE, (ft_ptr->self_num & 0xFF), 
                                            ft_ptr->ctrl.frame_num++, 0x00);
                        memset(ft_ptr->ctrl.ft_frame.data, 0xFE, sizeof(ft_ptr->ctrl.ft_frame.data));
                        ft_ptr->ctrl.ft_frame.data[1] = type;
                        ft_ptr->ctrl.ft_frame.data[0] = cmd;
                        memcpy((uint8_t*)&ft_ptr->ctrl.ft_frame.data+2, ft_ptr->ctrl.data_to_save, MIN(ft_ptr->ctrl.data_len, sizeof(ft_ptr->ctrl.ft_frame.data) - 2));
                        frame_crc16_calc((uint8_t*)&ft_ptr->ctrl.ft_frame);
                        ft_write_fifo(ft_ptr, (uint8_t*)&ft_ptr->ctrl.ft_frame);
                        break;
                    case FT_STATUS_SAVE_DATA_TYPE:  //! сохраняем готовый кадр
                        ft_write_fifo(ft_ptr, (uint8_t*)&ft_ptr->ctrl.data_to_save);
                        break;
                    default:
                        // невозможная ситуация
                        break;
                }
            }
            //  
            printf("%s: ft_<%d> st_<%d> func: t<%ld>c<%ld>ret<%ld>, data: ", now(), ft_ptr->self_num, ft_ptr->ctrl.step_num, (long)type, (long)cmd, (long)ft_ptr->ctrl.func_ret_val);
            printf_buff(ft_ptr->ctrl.data_to_save, MIN(10, ft_ptr->ctrl.data_len), '\n');
            // сброс значений данных для сохранения
            memset(ft_ptr->ctrl.data_to_save, 0xFE, sizeof(ft_ptr->ctrl.data_to_save));
        }
        else{
            ft_ptr->ctrl.func_ret_val = 0;
        }
        //
    }
    else{}
    return ft_ptr->ctrl.func_ret_val;
}

int32_t ft_function_retval_check(typeFT* ft_ptr)
{
    switch(ft_ptr->ctrl.func_ret_val)
    {
        case FT_FUN_RET_NOPE:
            //
            break;
        case FT_FUN_RET_BRANCH_RPT_BREAK:
            if(ft_ptr->ctrl.step_cast.fields.settings & FT_STATUS_BRANCH_RPT_BREAK){
                ft_ptr->ctrl.rpt_value = 0;
                printf("%s: ft_<%d> rpt break\n", now(), ft_ptr->self_num);
            }
            break;
        case FT_FUN_RET_BRANCH_FT_BREAK:
            if(ft_ptr->ctrl.step_cast.fields.settings & FT_FUN_RET_BRANCH_FT_BREAK){
                ft_ptr->ctrl.step_num = FT_LEN_STEP;
                printf("%s: ft_<%d> ft break\n", now(), ft_ptr->self_num);
            }
        break;
        default:
            //
        break;
    }
    return 0;
}

/**
 * @brief 
 * 
 * @param ft_ptr 
 * @return int8_t 
 */
int8_t ft_check_step_cast_crc(typeFT *ft_ptr)
{
    uint16_t crc16 = norby_crc16_calc(ft_ptr->ctrl.step_cast.array, sizeof(typeFTStep) - 2);
    if (crc16 == ft_ptr->ctrl.step_cast.fields.crc16) {
        return 1;
    }
    return 0;
}

/**
 * @brief Приведение переменных управления циклограммой к начальному значению
 * 
 * @param ft_ptr 
 */
void __ft_step_cast_reset(typeFT *ft_ptr)
{
    memset(&ft_ptr->ctrl, 0x00, sizeof(typeFTCtrl));
}


/**
 * @brief Регистрация функции для полетного задания
 * 
 * @param ft_ptr 
 * @param group_num Номер группы функций
 * @param func_num номер функции в группе
 * @param func указатель на функцию
 * @param ctrl_struct управляющая структура для регистрируемой функции
 * @return int8_t 1- функция зарегистрирована, 0 - функция не зарегистрирована
 */
int8_t ft_function_registration(typeFT *ft_ptr, uint8_t group_num, uint8_t func_num, int32_t (*func) (void*, uint8_t*, uint8_t*, uint8_t*), void* ctrl_struct)
{
    if (group_num < FT_FUNC_GROUP_NUM_MAX){
        if (func_num < FT_FUNCT_NUM_MAX){
            ft_ptr->f_group[group_num].func[func_num].func        = func;
            ft_ptr->f_group[group_num].func[func_num].ctrl_struct = ctrl_struct;
            // printf("\t ft_func_reg: t <%d>, cmd <%d> \n", group_num, func_num);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Загрузка массива полетного задания (массив в little-endian)
 * 
 * @param ft_ptr 
 * @param task_array массив данных с полетным заданием
 * @return int8_t -1 - некорректный массив с данными, >= 0  - количество рабочих шагов в полетном задании
 */
int8_t ft_load_task(typeFT *ft_ptr, uint8_t* task_array)
{
	uint8_t step_num = 0, total_step_num = 0;
	uint16_t crc16;
    typeFTStep step;
    //
    memset((uint8_t*)ft_ptr->task.array, 0x00, sizeof(ft_ptr->task));
    //
    for (step_num = 0; step_num < FT_LEN_STEP; step_num++){
        memcpy((uint8_t*)&step, task_array+FT_STEP_LEN_BYTE*step_num, sizeof(typeFTStep));
        // проверка валидности шага
        if(step.fields.label == FT_MARK){
            crc16 = norby_crc16_calc((uint8_t*)step.array, 62);
            if (step.fields.crc16 == crc16){
                total_step_num += 1;
            }
            else{
                return -1;
            }
        }
        else if((step.fields.label == 0x0000) && (step.fields.crc16 == 0x0000)){
            break;
        }
        else{
            return -1;
        }
    }
    // копирование только правильных подсчитанных шагов, если не было проблем с заголовком и crc16
    memcpy((uint8_t*)ft_ptr->task.array, task_array, total_step_num*FT_STEP_LEN_BYTE);
    //
    ft_refresh_go_to_parameters(ft_ptr);
    //
    return total_step_num;
}

/**
 * @brief Обновление параметров работы переходов goto
 * 
 * @param ft_ptr 
 */
void ft_refresh_go_to_parameters(typeFT *ft_ptr)
{
    uint8_t step_num = 0;
    // сборка информации о количествах переходов goto
    for(step_num=0; step_num < FT_LEN_STEP; step_num++){
        ft_ptr->ctrl.go_to_cnt[step_num] = ft_ptr->task.step[step_num].fields.go_cnt;
        ft_ptr->ctrl.go_to_max[step_num] = ft_ptr->task.step[step_num].fields.go_cnt;
    }
}

/**
 * @brief Выгрузка массива полетного задания
 * 
 * @param ft_ptr 
 * @return typeFTArray 
 */
typeFTArray ft_unload_task(typeFT *ft_ptr)
{
	return ft_ptr->task;
}


/**
 * @brief проверка полетного задания
 * 
 * @param ft_ptr 
 * @return int8_t -1 - некорректный массив с данными, 0  - количество рабочих шагов в полетном задании, > 0 - количество шагов
 */
int8_t ft_check_task(typeFT *ft_ptr)
{
	uint8_t step_num = 0, total_step_num = 0;
	uint16_t crc16;
    typeFTStep step;
    //
    for (step_num = 0; step_num < FT_LEN_STEP; step_num++){
        step = ft_ptr->task.step[step_num];
        // проверка валидности шага
        if(step.fields.label == FT_MARK){
            crc16 = norby_crc16_calc((uint8_t*)step.array, 62);
            if (step.fields.crc16 == crc16){
                total_step_num += 1;
            }
            else{
                return -1;
            }
        }
        else if((step.fields.label == 0x0000) && (step.fields.crc16 == 0x0000)){
            break;
        }
        else{
            return -1;
        }
    }
    // сборка информации о количествах переходов goto
    return total_step_num;
}

/**
  * @brief  запись измерения в fifo работаем с массивами по 128Б
	* @param  ft_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус записи: 1 - ок, <0 - ошибка
  */
int8_t ft_write_fifo(typeFT *ft_ptr, uint8_t* data)
{
	if (ft_ptr->fifo.rec_num >= FT_FIFO_DEPTH){
		return -1;
	}
	else{
		memcpy((uint8_t*)&ft_ptr->fifo.array[ft_ptr->fifo.rec_num][0], data, FT_FIFO_REC_LEN);
		ft_ptr->fifo.rec_num += 1;
		if (ft_ptr->fifo.rec_num > ft_ptr->fifo.rec_max) ft_ptr->fifo.rec_max = ft_ptr->fifo.rec_num;  //сбор информации о заполненности fifo
		return 1;
	}
}

/**
  * @brief  взятие измерения из fifo работаем с массивами по 128Б
	* @param  ft_ptr указатель на структуру управления
	* @param  data указатель на массив с кадром данных
	* @retval  статус: 0 - fifo-пуст, 1 - ок
  */
int8_t ft_read_fifo(typeFT* ft_ptr, uint8_t* data)
{
	if (ft_ptr->fifo.rec_num == 0){
		return 0;
	}
	else{
		ft_ptr->fifo.rec_num -= 1;
		memcpy((uint8_t*)data, &ft_ptr->fifo.array[0][0], FT_FIFO_REC_LEN);
		memmove(&ft_ptr->fifo.array[0][0], &ft_ptr->fifo.array[1][0], FT_FIFO_REC_LEN*ft_ptr->fifo.rec_num);
		memset((uint8_t*)&ft_ptr->fifo.array[ft_ptr->fifo.rec_num][0], 0x00, FT_FIFO_REC_LEN*(FT_FIFO_DEPTH - ft_ptr->fifo.rec_num));
		return 1;
	}
}

typeFTReport ft_create_report(typeFT* ft_ptr)
{
    ft_ptr->report.fields.self_id = ft_ptr->self_num;
    ft_ptr->report.fields.mode = ft_ptr->mode;
    ft_ptr->report.fields.step_num = ft_ptr->ctrl.step_num;
    ft_ptr->report.fields.function_info = ((ft_ptr->ctrl.step_cast.fields.type & 0xF) << 4) | ((ft_ptr->ctrl.step_cast.fields.cmd & 0xF) << 0);
    ft_ptr->report.fields.rpt_value = ft_ptr->ctrl.rpt_value;
    ft_ptr->report.fields.frame_num = ft_ptr->ctrl.frame_num;
    //
    return ft_ptr->report;
}

/**
 * @brief функция формирования шага полетного задания
 * 
 * @param step указатель на структуру шага
 * @param type тип команды
 * @param cmd номер команды
 * @param rpt_cnt счетчик повторов шага
 * @param delay_ms пауза до следующего шага
 * @param settings настройка работы (пока не используется)
 * @param data данные для функции
 */
void ft_create_ft_step(typeFTStep* step, uint8_t type, uint8_t cmd, uint16_t rpt_cnt, uint32_t delay_ms, uint16_t settings, uint8_t go_to, uint8_t go_cnt, uint8_t*data)
{
    step->fields.label      = FT_MARK;
    step->fields.type       = type;
    step->fields.cmd        = cmd;
    step->fields.rpt_cnt    = rpt_cnt;
    step->fields.go_to      = go_to;
    step->fields.go_cnt     = go_cnt;
    step->fields.delay_ms   = delay_ms;
    step->fields.settings   = settings;
    memset(step->fields.reserve, 0x00, sizeof(step->fields.reserve));
    memcpy(step->fields.data, data, sizeof(step->fields.data));
    step->fields.crc16      = norby_crc16_calc(step->array, sizeof(typeFTStep) - 2);
    // printf("\t ft_create_step: f_t <%d> cmd <%d>, rpt <%d>, pause <%d>\n", type, cmd, rpt_cnt, delay_ms);
}

