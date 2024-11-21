/**
  ******************************************************************************
  * @file           : lm_interfaces_data.c
  * @version        : v1.0
  * @brief          : содержит структуры данных для работы с БРК, а также функции формирования кадра по 128-байт
  * @author         : Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "lm_interfaces_data.h"

/**
  * @brief  получение маяка в режиме констант
  * @param  frame_ptr: указатель на кадр
  */
void fill_beacon_const_mode(type_LM_Beacon_Frame* frame_ptr)
{
  frame_ptr->lm_status = 0x3D3E;
  frame_ptr->pl_status = 0x00;
  frame_ptr->lm_temp = 0x57;
  frame_ptr->pl_power_switches = 0x5C;
}

/**
  * @brief  получение тми в режиме констант
  * @param  frame_ptr: указатель на кадр
  */
void fill_tmi_const_mode(type_LM_TMI_Data_Frame* frame_ptr)
{
  uint8_t i=0;
  //
  for(i=0; i<6; i++){
    frame_ptr->pl_status[i] = __REV16(((0x3D + (2*i+0)) << 8) + (0x3D + (2*i+1)));
  }
  for(i=0; i<7; i++){
    frame_ptr->pwr_inf[i].voltage = 0x49 + (2*i+0);
    frame_ptr->pwr_inf[i].current = 0x49 + (2*i+1);
  }
  for(i=0; i<5; i++){
    frame_ptr->temp[i] = 0x57 + i;
  }
  // TODO: возможно дополнить константы для других данных
}
