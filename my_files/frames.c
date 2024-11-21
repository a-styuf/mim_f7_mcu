/**
  ******************************************************************************
  * @file           : frames.c
  * @version        : v1.0
  * @brief          : содержит структуры данных для формирования кадров по 128 байт
  * @author         : Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "frames.h"

/**
  * @brief  инициализация структуры кадра из 128 байт
  * @param  header_ptr указатель на структуру с заголовком
  * @param  sat_id уникальный номер аппарата
  * @param  dev_id номер устройства
  * @param  type тип кадра: 0-одиночный кадр, 1-заголовок архивного кадра, 2-тело архивного кадра
  * @param  d_code тип данных (согласно принятому соглашению: например "Маяк" или "ТМИ")
  * @param  num если тип кадра одиночный, то не используется, если архивный:
  *                   - одиночный: не проверяется
  *                   - заголовок архива: количество кадров
  *                   - тело архива: номер кадра архива (нулевой - это заголовок, соответственно первый кадр тела - 1й)
  * @retval длина получившегося заголовка (для разных типов данных длина разная), 0 - ошибка
  */
uint8_t frame_create_header(uint8_t* header_ptr, uint16_t sat_id, uint8_t dev_id, uint8_t flag, uint8_t type, uint8_t d_code, uint16_t fr_num, uint16_t num)
{
	type_SingleFrame_Header* s_header;
	type_ArchHeadFrame_Header* ah_header;
	type_ArchBodyFrame_Header* ab_header;

  switch(type){
    case SINGLE_FRAME_TYPE:
      s_header = (type_SingleFrame_Header*)header_ptr;
      s_header->mark = FRAME_MARK;
      s_header->id_sat = sat_id;
      s_header->id_loc.fields.dev_id = dev_id & 0xF;
      s_header->id_loc.fields.flags = flag & 0x7;
      s_header->id_loc.fields.data_code = d_code & 0xFF;
      s_header->num = fr_num;
      s_header->time = clock_get_time_s();
      return sizeof(type_SingleFrame_Header);
    case ARCH_HEADER_FRAME_TYPE:
      ah_header = (type_ArchHeadFrame_Header*)header_ptr;
      ah_header->mark = FRAME_MARK;
      ah_header->id_sat = sat_id;
      ah_header->id_loc.fields.dev_id = dev_id & 0xF;
      ah_header->id_loc.fields.flags = flag & 0x7;
      ah_header->id_loc.fields.data_code = d_code & 0xFF;
      ah_header->num = fr_num;
      ah_header->time = clock_get_time_s();
      ah_header->arch_len = num;
      return sizeof(type_ArchHeadFrame_Header);
    case ARCH_BODY_FRAME_TYPE:
      ab_header = (type_ArchBodyFrame_Header*)header_ptr;
      ab_header->mark = FRAME_MARK;
      ab_header->id_sat = sat_id;
      ab_header->id_loc.fields.dev_id = dev_id & 0xF;
      ab_header->id_loc.fields.flags = (0x1 << 0) | (flag & 0x7);
      ab_header->id_loc.fields.data_code = d_code & 0xFF;
      ab_header->num = fr_num;
      ab_header->arch_num = num;
      return sizeof(type_ArchHeadFrame_Header);
  }
  return 0;
}

/**
  * @brief  подсчет и подстановка crc16 в готовый кадр из 128 байт
  * @param  frame_ptr: указатель на кадр
  */
void frame_crc16_calc(uint8_t* frame_ptr)
{
  uint16_t crc16;
  crc16 = norby_crc16_calc(frame_ptr, 126);
  frame_ptr[126] = (crc16 >> 8) & 0xFF;
  frame_ptr[127] = (crc16 >> 0) & 0xFF;
}

/**
  * @brief  проверка кадра на корректность
  * @param  frame указатель на начало кадра
  * @retval  >0 - кадр ок; 0 - некорректный кадр
  */
uint8_t frame_validate(uint8_t *frame)
{
  uint16_t label = *(uint16_t*)(frame);
  uint16_t crc16 = *(uint16_t*)(&frame[126]);
	if (label != 0x0FF1){  // проверка на метку кадра
		return 0;
	}
	else if(norby_crc16_calc((uint8_t*)&frame, 126) != crc16){ // проверка на контрольную сумму
		return 0;
	}
	return 1;
}

