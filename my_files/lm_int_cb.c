/**
  ******************************************************************************
  * @file           : lm_int_cb.c
  * @version        : v1.0
  * @brief          : расширение для main, содержащие обработчики для callback-ов интерфейсов МС и обработчики команд
  * @author         : Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "lm_int_cb.h"

extern type_LM_DEVICE lm;
extern type_MEM_CONTROL mem;

//*** CallBack ***//
/**
  * @brief  регистрация колбэков для обработки команд CAN
  */
void ProcCallbackCmds_Init(void)
{
  interface_cb_registration(&lm.interface, ID_IVAR_CMD, ProcCallbackCmds);
  interface_cb_registration(&lm.interface, ID_IVAR_CMDREG, ProcCallbackCmdRegs);
  interface_cb_registration(&lm.interface, ID_IVAR_EXTMEM, ProcCallbackExtMems);
  // interface_cb_registration(&lm.interface, ID_IVAR_DCR_INTERFACE, ProcCallbackDCRInterface);
  // interface_cb_registration(&lm.interface, ID_IVAR_ISS_INTERFACE, ProcCallbackISSInterface);
  interface_cb_registration(&lm.interface, ID_IVAR_FLASH_LOAD, ProcCallbackCAN_Flash);
}

//*** набор callback-функция для обработки команд интерфейса общения с внешним миром (*CallBackProc)(CAN_TypeDef *can_ref, typeIdxMask id, uint16_t leng, int state); ***//
/**
  * @brief  регистрация колбэков для обработки команд CAN
  * @param  can_ptr: указатель на структуру управления CAN
  * @param  id: номер переменной для обработки CB
  * @param  leng: длина данных для обработки
  * @param  state: тип CB - 0 до записи данных, 1 - после
  */
void ProcCallbackCmds(CAN_TypeDef *can_ptr, typeIdxMask id, uint16_t leng, int state) 
{
	//volatile int n;
	uint32_t i;
  if(state == 0)
    return;  //первый вызов тут не нужен
  if (id.std.RTR == 1) return;  //обработка чтения нам не нужна
  //if(can_ptr == CAN1) n = 1; else if(can_ptr == CAN2) n = 2; else n = 0;
	for (i=id.uf.Offset; i<(id.uf.Offset + leng); i++){
      cmd_process_cb(&lm.interface, i);
  }
}

/**
  * @brief  регистрация колбэков для обработки команд CAN
  * @param  can_ptr: указатель на структуру управления CAN
  * @param  id: номер переменной для обработки CB
  * @param  leng: длина данных для обработки
  * @param  state: тип CB - 0 до записи данных, 1 - после
  */
void ProcCallbackCmdRegs(CAN_TypeDef *can_ptr, typeIdxMask id, uint16_t leng, int state) 
{
	// volatile int n;
	uint32_t i;
  if(state == 0) return;  //первый вызов тут не нужен
  if (id.std.RTR == 1) return;  //обработка чтения нам не нужна
  // if(can_ptr == CAN1) n = 1; else if(can_ptr == CAN2) n = 2; else n = 0;
	for (i=id.uf.Offset; i<(id.uf.Offset + leng); i++){
      cmdreg_process_cb(&lm.interface, i);
  }
}

/**
 * @brief  регистрация колбэк функции для чтения данных из памяти
 * @param  can_ptr: указатель на структуру управления CAN
 * @param  id: номер переменной для обработки CB
 * @param  leng: длина данных для обработки
 * @param  state: тип CB - 0 до записи данных, 1 - после
 */
void ProcCallbackExtMems(CAN_TypeDef *can_ptr, typeIdxMask id, uint16_t leng, int state)
{
  // volatile int n;
  uint8_t offset, mem_num;
  if(state == 1) {
    return;  //обработка второго вызова не нужна
  }
  // if(can_ptr == CAN1) n = 1; else if(can_ptr == CAN2) n = 2; else n = 0;
  offset = id.uf.Offset % 128;
    if (id.uf.Offset % 8 == 0){
      mem_num = id.uf.Offset/128;
      if (mem_num == 0){  // Чтение в круг всей возможной памяти
        ext_mem_read_from_part_8b(&mem, offset, lm.interface.ext_mem.External_Mem_Full_Frame + offset, PART_ALL_MEM);
      }
      else if ((mem_num - 1) < PART_NUM){  // Чтение отдельных томов
        if((id.uf.Offset >= 128*(mem_num)) && (id.uf.Offset < 128*(mem_num+1))){
          ext_mem_read_from_part_8b(&mem, offset, lm.interface.ext_mem.External_Mem_Full_Frame + id.uf.Offset, mem_num - 1);
          // printf("rd_mem_n<%d> offs<%d> : ", mem_num, offset);
          // printf_buff(lm.interface.ext_mem.External_Mem_Full_Frame + id.uf.Offset, 2, '\n');
        }
      }
    }
}

/**
  * @brief  регистрация колбэков для обработки команд CAN
  * @param  can_ptr: указатель на структуру управления CAN
  * @param  id: номер переменной для обработки CB
  * @param  leng: длина данных для обработки
  * @param  state: тип CB - 0 до записи данных, 1 - после
  */
void ProcCallbackCAN_Flash(CAN_TypeDef *can_ptr, typeIdxMask id, uint16_t leng, int state) 
{
  // volatile int n;
  CallbackCAN_Flash(can_ptr, id, leng, state);
  // if(can_ptr == CAN1) n = 1; else if(can_ptr == CAN2) n = 2; else n = 0;
}

//*** Cmds process function ***//
/**
  * @brief  функция для создания псевдопотока для выполнения продолжительной команды: test_led
  *         - раз в 2 секунды увеличивает скважность на 2 бит (100% - 255 бит)
  *         - полный цикл 200 секунд: от отсутствия мигания до постоянного горения
  * @param  period_ms: период вызова обработчика в псевдопотоке
  */
void cmd_process_test_led(uint8_t mode, uint32_t period_ms)
{
  uint8_t cmd_code = CMD_DBG_LED_TEST;
  //*** Прерывание работы ***//
  if (mode == MODE_CANCEL){
      cmd_set_status(&lm.interface, cmd_code, CMD_STATUS_CANCEL);
      lm.cmd_ctrl[cmd_code].main_counter = 0;
      lm.cmd_ctrl[cmd_code].time_ms = 0;
      lm.cmd_ctrl[cmd_code].point_time_ms = 0;
      lm.cmd_ctrl[cmd_code].ena = 0;
      return;
    }
  else{
    //*** Запуск работы ***//
    if ((mode == MODE_START) && (lm.cmd_ctrl[cmd_code].ena == 0)){
      lm.cmd_ctrl[cmd_code].main_counter = 0;
      lm.cmd_ctrl[cmd_code].time_ms = 0;
      lm.cmd_ctrl[cmd_code].point_time_ms = 0;
      lm.cmd_ctrl[cmd_code].ena = 1;
      cmd_set_status(&lm.interface, cmd_code, CMD_STATUS_START);
      return;
    }
    //*** Собственно тело процесса ***//
    else if (lm.cmd_ctrl[cmd_code].ena == 1){
      lm.cmd_ctrl[cmd_code].time_ms += period_ms;
      lm.cmd_ctrl[cmd_code].point_time_ms += period_ms;
      if (lm.cmd_ctrl[cmd_code].point_time_ms >= 2000){
        lm.cmd_ctrl[cmd_code].point_time_ms = 0;
        lm.cmd_ctrl[cmd_code].main_counter += 2;
        led_alt_setup(&mcu_state_led, LED_BLINK, 500, (255 - (lm.cmd_ctrl[cmd_code].main_counter & 0xFF)), 2000);
      }
      if (lm.cmd_ctrl[cmd_code].main_counter >= 254){
        //
        lm.cmd_ctrl[cmd_code].ena = 2; // отправляем на окончание работы
      }
      cmd_set_status(&lm.interface, cmd_code, (lm.cmd_ctrl[cmd_code].main_counter >> 2) & 0x7F);
    }
    //*** Окончание работы ***//
    else if (lm.cmd_ctrl[cmd_code].ena == 2){
      lm.cmd_ctrl[cmd_code].main_counter = 0;
      lm.cmd_ctrl[cmd_code].time_ms = 0;
      lm.cmd_ctrl[cmd_code].point_time_ms = 0;
      lm.cmd_ctrl[cmd_code].ena = 0;
      cmd_set_status(&lm.interface, cmd_code, CMD_STATUS_FINISH);
    }
  }
}
