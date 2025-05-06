/**
  ******************************************************************************
  * @file           : ext_mem.c
  * @version        : v1.0
  * @brief          : надстройка над CubeMX HAL для работы с внешней SPI памятью
  * @author         : Стюф Алексей/Alexey Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "ext_mem.h"
#include <stdio.h>

/**
  * @brief  инициализация работы с CY15X104QN: специально использую блокирующий режим
  * @param  mem_ptr: структура для управления памятью
  * @retval статус ошибки: 1 - все хорошо, 0 - есть ошибка
  */
int8_t ext_mem_init(type_MEM_CONTROL* mem_ptr, SPI_HandleTypeDef* spi_ptr, uint16_t sat_id, uint8_t dev_id)
{
  int8_t report = 0;
  uint32_t start_addr = 0;
  mem_ptr->read_ptr = 0;
  mem_ptr->sat_id = sat_id;
  mem_ptr->dev_id = dev_id;
  GPIO_TypeDef* port[CY15B104_MEM_NUM] = {GPIOD, GPIOD};
  uint8_t line[CY15B104_MEM_NUM] = {8, 9};
  // инициализируем память
  for(uint8_t mem_num = 0; mem_num<CY15B104_MEM_NUM; mem_num ++)
  {
    report |= (cy15_init(&mem_ptr->cy15b104[mem_num], spi_ptr, port[mem_num], line[mem_num]) & 0x01) << mem_num;
  }
  mem_ptr->fram_load_status = report;
  printf("%s: FRAM init done. Result: <0x%02X>  (0 isn't OK, <0x%02X> - is OK)\n", now(), report, ((1<<CY15B104_MEM_NUM)-1));
  // инициализируем блоки памяти для периферии
  start_addr = part_rel_init(&mem_ptr->part[PART_GENERAL], PART_GENERAL, PART_MODE_SMART_COIL_WRITE, PART_FULL_VOL_REL, PART_GENERAL_REL, start_addr);
  start_addr = part_rel_init(&mem_ptr->part[PART_PL1], PART_PL1, PART_MODE_SMART_COIL_WRITE, PART_FULL_VOL_REL, PART_PL1_REL, start_addr);
  start_addr = part_rel_init(&mem_ptr->part[PART_PL2], PART_PL2, PART_MODE_SMART_COIL_WRITE, PART_FULL_VOL_REL, PART_PL2_REL, start_addr);
  start_addr = part_rel_init(&mem_ptr->part[PART_PL3], PART_PL3, PART_MODE_SMART_COIL_WRITE, PART_FULL_VOL_REL, PART_PL3_REL, start_addr);
  start_addr = part_rel_init(&mem_ptr->part[PART_PL4], PART_PL4, PART_MODE_SMART_COIL_WRITE, PART_FULL_VOL_REL, PART_PL4_REL, start_addr);
  start_addr = part_const_init(&mem_ptr->part[PART_FT0], PART_FT0, PART_MODE_REWRITE, PART_FT0_VOL, start_addr);
  start_addr = part_const_init(&mem_ptr->part[PART_FT1], PART_FT1, PART_MODE_REWRITE, PART_FT1_VOL, start_addr);
  start_addr = part_const_init(&mem_ptr->part[PART_FT2], PART_FT2, PART_MODE_REWRITE, PART_FT2_VOL, start_addr);
  start_addr = part_const_init(&mem_ptr->part[PART_FT3], PART_FT3, PART_MODE_REWRITE, PART_FT3_VOL, start_addr);
  // чтение всех сохраненных данных с сохранением в оперативной памяти
  report = ext_mem_load_full_cfg_frames(mem_ptr);
  printf("%s: Cfg frames were loaded. Result: <%d>  (0 is OK)\n", now(), report);
  //
  return 0;
}

/**
 * @brief Выгрузка параметров работы томов памяти во внешние кадры
 * 
 * @param mem_ptr 
 * @return int8_t всегда 0
 */
int8_t ext_mem_get_mem_cfg_frames(type_MEM_CONTROL* mem_ptr,typeSingleFrameStruct *mem_cfg_frame_1_ptr)
{
	//ext_mem_form_cfg_frame(mem_ptr);
  *mem_cfg_frame_1_ptr = mem_ptr->self_cfg.fr1.frame;
  return 0;
}


/**
 * @brief Формирование кадров для сохранения в память конфигураций (заголовок и crc)
 * 
 * @param mem_ptr 
 * @return int8_t всегда 0
 */
int8_t ext_mem_form_cfg_frame(type_MEM_CONTROL* mem_ptr)
{
	frame_create_header((uint8_t*)&mem_ptr->self_cfg.fr1.frame.header, mem_ptr->sat_id, mem_ptr->dev_id, 0x00, SINGLE_FRAME_TYPE, MEM_DATA_TYPE_SELF_CFG_1, mem_ptr->self_frame_num++, 0x00);
	//
	memset((uint8_t*)mem_ptr->self_cfg.fr1.fields.reserve, 0xFE, sizeof(mem_ptr->self_cfg.fr1.fields.reserve));
  //
  ext_mem_full_update_cfg(mem_ptr);
	//
	frame_crc16_calc((uint8_t*)&mem_ptr->self_cfg.fr1);
  //
  return 0;
}

/**
 * @brief Обновление конфигурации всех томов, включаю всю память
 * 
 * @param mem_ptr 
 * @return int8_t всегда 0
 */
int8_t ext_mem_full_update_cfg(type_MEM_CONTROL* mem_ptr)
{
  ext_mem_part_update_cfg(mem_ptr, PART_ALL_MEM);
  for(uint8_t part_num = 0; part_num < PART_NUM; part_num++){
    ext_mem_part_update_cfg(mem_ptr, part_num);
  }
  return 0;
}

/**
 * @brief обновление конфигурации отдельного тома
 * 
 * @param mem_ptr 
 * @param part_num номер тома (до PART_NUM) или PART_ALL_MEM = 127 
 * @return int8_t 0 - норма, -1 - некорректны номер тома
 */
int8_t ext_mem_part_update_cfg(type_MEM_CONTROL* mem_ptr, uint8_t part_num)
{
  if(part_num < PART_NUM){
    mem_ptr->self_cfg.fr1.fields.part_cfg[part_num].write_ptr = part_get_rd_ptr(&mem_ptr->part[part_num]);
    mem_ptr->self_cfg.fr1.fields.part_cfg[part_num].read_ptr = part_get_wr_ptr(&mem_ptr->part[part_num]);
    return 0;
  }
  else if(part_num == PART_ALL_MEM){
    mem_ptr->self_cfg.fr1.fields.full_cfg.write_ptr = 0x00;
    mem_ptr->self_cfg.fr1.fields.full_cfg.read_ptr = mem_ptr->read_ptr;
    return 0;
  }
  else{
    return -1;
  }
}

/**
 * @brief Применение полной конфигурации к определенному разделу памяти.
 *
 * Эта функция применяет полные настройки конфигурации к указанному
 * разделу памяти.
 *
 * @param mem_ptr Указатель на структуру управления памятью.
 * @param part_num Номер раздела, к которому будет применена конфигурация.
 * @return int8_t Возвращает 0 при успешном выполнении или отрицательный код ошибки при сбое.
 */
int8_t ext_mem_part_apply_cfg(type_MEM_CONTROL* mem_ptr, typeSingleFrameStruct fr1)
{
  type_MEM_CFG tmp_cfg;
  tmp_cfg.fr1.frame = fr1;
  if (frame_validate((uint8_t*)&tmp_cfg.fr1.frame)){
    // full mem line read ptr
    mem_ptr->read_ptr = tmp_cfg.fr1.fields.full_cfg.read_ptr;
    // part cfg apply
    for (uint8_t part_num = 0; part_num < 13; part_num++){
      ext_mem_set_rd_ptr_for_part(mem_ptr, part_num, tmp_cfg.fr1.fields.part_cfg[part_num].read_ptr);
      ext_mem_set_wr_ptr_for_part(mem_ptr, part_num, tmp_cfg.fr1.fields.part_cfg[part_num].write_ptr);
    }
  }
  else{
    //
  }
	//
	ext_mem_form_cfg_frame(mem_ptr);
	//
	return 0;
}

/**
 * @brief Загрузка всех настроек МС в оперативную память
 * 
 * @param mem_ptr 
 * @return int8_t всегда 0
 */
int8_t ext_mem_load_full_cfg_frames(type_MEM_CONTROL* mem_ptr)
{ 
  int8_t retval = 0;
  for(uint8_t cfg_fr = 0; cfg_fr<EXT_MEM_SETTINGS_FRAME_NUM; cfg_fr++) {
    mem_ptr->loaded_cfg.cfg_fr_ld_status[cfg_fr] = ext_mem_rd_param(mem_ptr, (uint8_t*)&mem_ptr->loaded_cfg.cfg_frames[cfg_fr], cfg_fr);
    if (mem_ptr->loaded_cfg.cfg_fr_ld_status[cfg_fr] == 0){
      retval += -1;
    }
  }
  return retval;
}

/**
 * @brief Получает конфигурационный кадр из внешней памяти.
 *
 * @param mem_ptr Указатель на структуру управления внешней памятью.
 * @param cfg_fr_num Номер конфигурационного кадра.
 * @param cfg_fr Структура для хранения кадра.
 * @return int8_t 0 - успешно, -1 - ошибка номера конфигурации, -2 - ошибка кадра
 */
int8_t ext_mem_get_cfg_frame(type_MEM_CONTROL* mem_ptr, uint8_t cfg_fr_num, typeSingleFrameStruct* cfg_fr_ptr)
{
  if (cfg_fr_num < EXT_MEM_SETTINGS_FRAME_NUM) {
    if (frame_validate((uint8_t*)&mem_ptr->loaded_cfg.cfg_frames[cfg_fr_num])){
      memcpy((uint8_t*)cfg_fr_ptr, &mem_ptr->loaded_cfg.cfg_frames[cfg_fr_num], sizeof(typeSingleFrameStruct));
    }
    else{
      return -2;
    }
    return 0;
  } else {
    return -1;
  }
}



/**
  * @brief  запись 128-ми байтового блока в произвольное место памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  frame_addr: адрес кадра в памяти
  * @param  buff: указатель на блок памяти
  */
void ext_mem_any_write(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t* buff)
{
  uint32_t b_addr=0;
  for(uint8_t i=0; i<CY15B104_MEM_NUM; i++){
    if (((i*SINGLE_MEM_VOL_FRAMES) <= frame_addr) && (frame_addr < ((i+1)*SINGLE_MEM_VOL_FRAMES))){
      b_addr = 128*(frame_addr - (i*SINGLE_MEM_VOL_FRAMES));
      //
      cy15_write(&mem_ptr->cy15b104[i], b_addr, buff, 128);
      //
			break;
    }
  }
}

/**
  * @brief  чтение 128-ми байтового блока из произвольного места памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  frame_addr: адрес кадра в памяти
  * @param  buff: указатель на блок памяти
  */
void ext_mem_any_read(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t* buff)
{
  uint32_t b_addr=0;
  for(uint8_t i=0; i<CY15B104_MEM_NUM; i++){
    if (((i*SINGLE_MEM_VOL_FRAMES) <= frame_addr) && (frame_addr < ((i+1)*SINGLE_MEM_VOL_FRAMES))){
      b_addr = 128*(frame_addr - (i*SINGLE_MEM_VOL_FRAMES));
      //
      // set_lock();
      cy15_read(&mem_ptr->cy15b104[i], b_addr, buff, 128);
      // release_lock();
      //
			break;
    }
  }
}

/**
  * @brief  чтение 8-ми байтового блока из произвольного места памяти по адресу кадра и смещению внутри кадра (для передачи по CAN)
  * @param  mem_ptr: структура для управления памятью
  * @param  frame_addr: адрес кадра в памяти
  * @param  offset: смещение внутри кадра (т.к. кадр 128 - байт, читаемы блок 8 - байт, offset не более 120 и кратен 8-ми)
  * @param  buff: указатель на блок памяти (не менее 8-ми байт)
  */
void ext_mem_any_read_8b_block(type_MEM_CONTROL* mem_ptr, uint32_t frame_addr, uint8_t offset, uint8_t* buff)
{
  uint32_t b_addr=0;
  for(uint8_t i=0; i<CY15B104_MEM_NUM; i++){
    if (((i*SINGLE_MEM_VOL_FRAMES) <= frame_addr) && (frame_addr < ((i+1)*SINGLE_MEM_VOL_FRAMES))){
      if ((offset % 8 != 0) && (offset <=120)){
        break;
      }
      b_addr = 128*(frame_addr - (i*SINGLE_MEM_VOL_FRAMES));
      b_addr += offset;
      cy15_read(&mem_ptr->cy15b104[i], b_addr, buff, 8);
			break;
    }
  }
}

/**
  * @brief  последовательное чтение 128-ми байтового блока из памяти по read_ptr
  * @param  mem_ptr: структура для управления памятью
  * @param  buff: указатель на блок памяти
  */
void ext_mem_any_line_read(type_MEM_CONTROL* mem_ptr, uint8_t* buff)
{
  uint32_t b_addr=0;
  for(uint8_t i=0; i<CY15B104_MEM_NUM; i++){
    if (((i*SINGLE_MEM_VOL_FRAMES) <= mem_ptr->read_ptr) && (mem_ptr->read_ptr < ((i+1)*SINGLE_MEM_VOL_FRAMES))){
      b_addr = 128*(mem_ptr->read_ptr - (i*SINGLE_MEM_VOL_FRAMES));
      cy15_read(&mem_ptr->cy15b104[i], b_addr, buff, 128);
			break;
    }
  }
  if (mem_ptr->read_ptr >= FULL_MEM_VOL_FRAMES) mem_ptr->read_ptr = 0;
  else mem_ptr->read_ptr += 1;
}

/**
  * @brief  чтение по 8 байт из всей памяти по указателю чтения и offset, при offset=120 (последняя область из 8-байт) происходит инкрементация указателя чтения
  * @param  mem_ptr: структура для управления памятью
  * @param  offset: сдвиг внутри кадра
  * @param  buff: указатель на блок памяти
  */
void ext_mem_any_line_read_8b_block(type_MEM_CONTROL* mem_ptr, uint8_t offset, uint8_t* buff)
{
  uint32_t b_addr=0;
  for(uint8_t i=0; i<CY15B104_MEM_NUM; i++){
    if (((i*SINGLE_MEM_VOL_FRAMES) <= mem_ptr->read_ptr) && (mem_ptr->read_ptr < ((i+1)*SINGLE_MEM_VOL_FRAMES))){
      if ((offset % 8 != 0) && (offset <=120)){
        break;
      }
      b_addr = 128*(mem_ptr->read_ptr - (i*SINGLE_MEM_VOL_FRAMES));
      b_addr += offset;
      cy15_read(&mem_ptr->cy15b104[i], b_addr, buff, 8);
			break;
    }
  }
  if (offset == 120){
    if (mem_ptr->read_ptr >= FULL_MEM_VOL_FRAMES) mem_ptr->read_ptr = 0;
    else mem_ptr->read_ptr += 1;
  }
}

/**
  * @brief  блокирующая неразрушающая проверка памяти 128-ми байтными блоками
  * @param  mem_ptr: структура для управления памятью
  * @param  symbol: символ для проверки
  * @param  retval: -1 - ошибка проверки, остальное - адрес блока с ошибкой 
  */
int32_t ext_mem_check(type_MEM_CONTROL* mem_ptr, uint8_t symbol)
{
  uint8_t buff[128], control_buff[128], read_buf[128];
  memset(control_buff, symbol , 128);
  for(uint32_t addr=0; addr < FULL_MEM_VOL_FRAMES; addr++){
    ext_mem_any_read(mem_ptr, addr, buff);
    ext_mem_any_write(mem_ptr, addr, control_buff);
    ext_mem_any_read(mem_ptr, addr, read_buf);
    ext_mem_any_write(mem_ptr, addr, buff);
    if (memcmp(control_buff, read_buf, 128)) {
      return addr;
    }
  }
  return -1;
}

/**
  * @brief  запись параметров (128-байт)
  * @param  mem_ptr: структура для управления памятью
  * @param  param: указатель на структуру с параметрами
  * @param  settings_frame_num: номер кадра для записи параметров сохранения
  * @param  retval: 1 - параметры записаны, 0 - ошибка записи, -1 - ошибка параметров
  */
int32_t ext_mem_wr_param(type_MEM_CONTROL* mem_ptr, uint8_t *param, uint8_t settings_frame_num)
{
  uint8_t buff[128];
  memcpy(buff, param, 128);
  if(settings_frame_num < EXT_MEM_SETTINGS_FRAME_NUM){
    for(uint8_t num=0; num < CY15B104_MEM_NUM; num++){
      ext_mem_any_write(mem_ptr, num*SINGLE_MEM_VOL_FRAMES + settings_frame_num, buff);
    }
  }
  else{
    return -1;
  }
  return 1;
}

/**
  * @brief  чтение параметров (128-байт)
  * @param  mem_ptr: структура для управления памятью
  * @param  param: указатель на структуру с параметрами
  * @param  settings_frame_num: номер кадра для записи параметров сохранения
  * @param  retval: >0 - номер памяти из которой прочитаны параметры, 0 - ошибка чтения, -1 - ошибка параметров
  */
int32_t ext_mem_rd_param(type_MEM_CONTROL* mem_ptr, uint8_t *param, uint8_t settings_frame_num)
{
  uint8_t buff[128];
	// uint16_t crc16=0;
  if(settings_frame_num < EXT_MEM_SETTINGS_FRAME_NUM){
    for(uint8_t num=0; num < CY15B104_MEM_NUM; num++){
      ext_mem_any_read(mem_ptr, num*SINGLE_MEM_VOL_FRAMES + settings_frame_num, buff);
      memcpy(param, buff, 128);
      if(frame_validate(param)){
        return num+1;
      }
    }
    return 0;
  }
  else {
    return -1;
  }
}

/**
  * @brief  запись кадра с данными
  * @param  mem_ptr: структура для управления памятью
  * @param  addr: номер кадра в пространстве кадров с данными
  * @param  frame: указатель на структуру с кадром
  */
void ext_mem_wr_data_frame(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t *frame)
{
  uint32_t real_addr;
  real_addr = addr +
              EXT_MEM_SETTINGS_FRAME_NUM + 
              EXT_MEM_SETTINGS_FRAME_NUM * ((addr+EXT_MEM_SETTINGS_FRAME_NUM)/SINGLE_MEM_VOL_FRAMES);              ;
  ext_mem_any_write(mem_ptr, real_addr, frame);
}

/**
  * @brief  чтение кадров с данными
  * @param  mem_ptr: структура для управления памятью
  * @param  addr: номер кадра в пространстве кадров с данными
  * @param  frame: указатель на структуру с кадром
  */
void ext_mem_rd_data_frame(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t *frame)
{
  uint32_t real_addr;
  real_addr = addr +
              EXT_MEM_SETTINGS_FRAME_NUM + 
              EXT_MEM_SETTINGS_FRAME_NUM * ((addr+EXT_MEM_SETTINGS_FRAME_NUM)/SINGLE_MEM_VOL_FRAMES); 
  ext_mem_any_read(mem_ptr, real_addr, frame);
}

/**
  * @brief  чтение кадров с данными по 8 байт (для CAN)
  * @param  mem_ptr: структура для управления памятью
  * @param  addr: номер кадра в пространстве кадров с данными
  * @param  offset: номер кадра в пространстве кадров с данными
  * @param  frame: указатель на структуру с кадром
  */
void ext_mem_rd_data_frame_8b_block(type_MEM_CONTROL* mem_ptr, uint32_t addr, uint8_t offset, uint8_t *frame)
{
  uint32_t real_addr;
  real_addr = addr +
              EXT_MEM_SETTINGS_FRAME_NUM + 
              EXT_MEM_SETTINGS_FRAME_NUM * ((addr+EXT_MEM_SETTINGS_FRAME_NUM)/SINGLE_MEM_VOL_FRAMES); 
  ext_mem_any_read_8b_block(mem_ptr, real_addr, offset, frame);
}

/**
  * @brief  запись кадра с данными в определенную область памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  frame: указатель на структуру с кадром
  * @param  part_num: номер тома памяти
  */
void ext_mem_wr_frame_to_part(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t part_num)
{
  ext_mem_wr_data_frame(mem_ptr, mem_ptr->part[part_num].write_ptr + mem_ptr->part[part_num].start_frame_num, frame);
  part_wr_rd_ptr_calc(&mem_ptr->part[part_num], MODE_WRITE);
  ext_mem_form_cfg_frame(mem_ptr);
}

/**
  * @brief  чтение кадра с данными из определенной области памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  frame: указатель на структуру с кадром
  * @param  part_num: номер тома памяти
  */
void ext_mem_rd_frame_from_part(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t part_num)
{
  ext_mem_rd_data_frame(mem_ptr, mem_ptr->part[part_num].read_ptr + mem_ptr->part[part_num].start_frame_num, frame);
  part_wr_rd_ptr_calc(&mem_ptr->part[part_num], MODE_READ);
  ext_mem_form_cfg_frame(mem_ptr);
}

/**
  * @brief  запись кадра в блок по адресу
  * @param  mem_ptr: структура для управления памятью
  * @param  frame: указатель на структуру с кадром
  * @param  fr_addr: адрес кадра в блоке
  * @param  part_num: номер тома памяти
  */
void ext_mem_rd_frame_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t fr_addr, uint8_t part_num)
{
  ext_mem_rd_data_frame(mem_ptr, mem_ptr->part[part_num].start_frame_num + fr_addr, frame);
}

/**
  * @brief  запись кадра из блока по адресу
  * @param  mem_ptr: структура для управления памятью
  * @param  frame: указатель на структуру с кадром
  * @param  fr_addr: адрес кадра в блоке
  * @param  part_num: номер тома памяти
  */
void ext_mem_wr_frame_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t *frame, uint8_t fr_addr, uint8_t part_num)
{
  ext_mem_wr_data_frame(mem_ptr, mem_ptr->part[part_num].start_frame_num + fr_addr, frame);
}

/**
 * @brief чтение произвольного объема информации (кратного размеру кадра) из тома памяти. Чтение производится циклично
 * 
 * @param mem_ptr структура для управления памятью
 * @param part_num  номер тома
 * @param buff указатель на массив данных
 * @param len длина массива данных в кадрах
 * @param fr_addr номер кадра с которого начинается запись
 */
void ext_mem_rd_buff_from_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint8_t *buff, uint16_t len, uint16_t fr_addr)
{
  uint8_t i=0, current_frame_addr;
  current_frame_addr = fr_addr;
  for(i=0; i<len; i++){
    //
    if (current_frame_addr > mem_ptr->part[part_num].finish_frame_num) {
      current_frame_addr -= mem_ptr->part[part_num].full_frame_num;
    }
    ext_mem_rd_frame_from_part_by_addr(mem_ptr, buff + 128*i, current_frame_addr, part_num);
    // printf("rd_fr_n<%d> to p_n<%d> : ", current_frame_addr, part_num);
    // printf_buff(buff + 128*i, 2, '\n');
    current_frame_addr++;
  }
}

/**
 * @brief запись произвольного объема информации (кратного размеру кадра) в том памяти. Запись производится циклично.
 * 
 * @param mem_ptr структура для управления памятью
 * @param part_num  номер тома
 * @param buff указатель на массив данных
 * @param len длина массива данных в кадрах
 * @param fr_addr номер кадра с которого начинается запись
 */
void ext_mem_wr_buff_to_part_by_addr(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint8_t *buff, uint16_t len, uint16_t fr_addr)
{
  uint8_t i=0, current_frame_addr;
  current_frame_addr = fr_addr;
  for(i=0; i<len; i++){
	//
	if (current_frame_addr > mem_ptr->part[part_num].finish_frame_num) {
	  current_frame_addr -= mem_ptr->part[part_num].full_frame_num;
	}
    ext_mem_wr_frame_from_part_by_addr(mem_ptr, buff + 128*i, current_frame_addr, part_num);
    // printf("wr_fr_n<%d> to p_n<%d> : ", current_frame_addr, part_num);
    // printf_buff(buff + 128*i, 2, '\n');
	current_frame_addr++;
	//
  }
}

/**
  * @brief  блокирующее стирание всей памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  symbol: символ для записи
  */
void ext_mem_full_erase(type_MEM_CONTROL* mem_ptr, uint8_t symbol)
{
  uint8_t buff[128];
  memset(buff, symbol, 128);
  for(uint32_t addr=0; addr < FULL_MEM_VOL_FRAMES; addr++){
    *(uint32_t*)&buff[0] = addr;
    ext_mem_any_write(mem_ptr, addr, buff);
  }
}

/**
  * @brief  форматирование тома памяти, блокирующее
  * @param  mem_ptr: структура для управления памятью
  * @param  part_num: номер тома памяти
  */
void ext_mem_format_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num)
{
  uint8_t frame[128] = {0};
  memset(frame, 0xFE, 128);
  *(uint16_t*)&frame[0] = part_num;
  for(uint32_t addr = 0; addr < mem_ptr->part[part_num].full_frame_num; addr++){
    *(uint32_t*)&frame[2] = addr;
    ext_mem_wr_data_frame(mem_ptr, addr + mem_ptr->part[part_num].start_frame_num, frame);
  }
  mem_ptr->part[part_num].write_ptr = 0;
  mem_ptr->part[part_num].read_ptr = 0;
}

/**
  * @brief  установка указателей записи для частей памяти
  * @param  mem_ptr структура для управления памятью
  * @param  part_num номер тома памяти
  * @param  wr_ptr указатель записи
  * @retval 1 - успешно установили указатель, 0 - ошибка
  */
int8_t ext_mem_set_wr_ptr_for_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint32_t wr_ptr)
{
  if (part_num < PART_NUM){
    if (wr_ptr >= (mem_ptr->part[part_num].full_frame_num)){
      mem_ptr->part[part_num].write_ptr = mem_ptr->part[part_num].full_frame_num - 1;
      wr_ptr =  mem_ptr->part[part_num].full_frame_num - 1;

      return 0;
    }
    else{
      mem_ptr->part[part_num].write_ptr = wr_ptr;
      return 1;
    }
  }
  else{
    part_num = 0xFF;
    wr_ptr = 0;
    return 0;
  }
}

/**
  * @brief  установка указателей чтения для частей памяти
  * @param  mem_ptr структура для управления памятью
  * @param  part_num указатель номера тома памяти, 0x7F - вся память
  * @param  rd_ptr указатель чтения
  * @retval 1 - успешно установили указатель, 0 - ошибка
  */
int8_t ext_mem_set_rd_ptr_for_part(type_MEM_CONTROL* mem_ptr, uint8_t part_num, uint32_t rd_ptr)
{
  if (part_num < PART_NUM){
    if (rd_ptr >= (mem_ptr->part[part_num].full_frame_num)){
      mem_ptr->part[part_num].read_ptr = mem_ptr->part[part_num].full_frame_num - 1;
      rd_ptr =  mem_ptr->part[part_num].full_frame_num - 1;
      return 0;
    }
    else{
      mem_ptr->part[part_num].read_ptr = rd_ptr;
      return 1;
    }
  }
  else if (PART_ALL_MEM == part_num){
    if (rd_ptr >= FRAME_MEM_VOL_FRAMES){
      mem_ptr->read_ptr = FRAME_MEM_VOL_FRAMES - 1;
      rd_ptr = FRAME_MEM_VOL_FRAMES - 1;
      return 0;
    }
    else{
      mem_ptr->read_ptr = rd_ptr;
      return 1;
    }
  }
  else{
    part_num = 0xFF;
    rd_ptr = 0;
    return 0;
  }
}

/**
  * @brief  чтение данных произвольной длины из памяти
  * @param  mem_ptr: структура для управления памятью
  * @param  offset: смещение адреса внутри кадра, если  = 0 - инкриминируется указатель чтения
  * @param  part_num: указатель номера тома памяти, 0x7F - вся память
  * @param  rd_ptr: значение указателя чтения для установки
  * @retval 1 - успешно установили указатель, 0 - ошибка
  */
void ext_mem_read_from_part_8b(type_MEM_CONTROL* mem_ptr, uint8_t offset, uint8_t *frame_part, uint8_t part_num)
{
	uint32_t frame_abs_addr;
  switch(part_num){
    case PART_ALL_MEM:
      ext_mem_any_line_read_8b_block(mem_ptr, offset, frame_part);
      break;
    default:
      if (part_num < PART_NUM){
        frame_abs_addr = mem_ptr->part[part_num].read_ptr + mem_ptr->part[part_num].start_frame_num;
        ext_mem_rd_data_frame_8b_block(mem_ptr, frame_abs_addr, offset, frame_part);
        if(offset == 120){ //если запрашиваем последнюю область памяти, то обрабатываем указатели чтения/записи
          part_wr_rd_ptr_calc(&mem_ptr->part[part_num], MODE_READ);
          ext_mem_form_cfg_frame(mem_ptr);
        }
      }
      break;
  }
}

/**
 * @brief Возвращает переменную со статусом тома памяти
 * 
 * @param mem_ptr 
 * @param part_num 
 * @return type_MEM_PART_REPORT 
 */
type_MEM_PART_REPORT ext_mem_get_part_report(type_MEM_CONTROL* mem_ptr, uint8_t part_num)
{
  type_MEM_PART_REPORT report = { .full_ptr = 0xFEFE,
                                  .rd_ptr = 0xFEFE,
                                  .wr_ptr = 0xFEFE};
  if (part_num < PART_NUM){
    report = part_get_report(&mem_ptr->part[part_num]);
  }
  return report;
}

///*** Работа с блоками памяти ***///

/**
  * @brief  инициализация работы с отдельным куском памяти c относительным распределением памяти
  * @param  part_ptr: структура для управления памятью
  * @param  mode: тип чтения-записи в блок памяти
  * @param  full_rel_vol: общий относительный объем памяти
  * @param  rel_vol: относительный объем блока памяти
  * @param  start_frame_addr: адрес блока памяти в общем пространстве доступных кадров
  * @retval адрес начала следующего тома области в кадрах
  */
uint32_t part_rel_init(type_MEM_PART_CONTROL* part_ptr, uint8_t self_num, uint8_t mode, uint16_t full_rel_vol, uint16_t rel_vol, uint32_t start_frame_addr)
{
  part_ptr->self_num = self_num;
  //
  part_ptr->start_frame_num = start_frame_addr;
  part_ptr->full_frame_num = (1. * (FRAME_MEM_VOL_FRAMES - PART_FULL_CONST) * rel_vol) / full_rel_vol;
  part_ptr->finish_frame_num = part_ptr->start_frame_num + part_ptr->full_frame_num - 1;
  // 
  part_ptr->write_ptr = 0;
  part_ptr->read_ptr = 0;
  part_ptr->mode = mode;
	//
	return part_ptr->finish_frame_num + 1;
}

/**
  * @brief  инициализация работы с отдельным куском памяти c постоянным распределением памяти
  * @param  part_ptr: структура для управления памятью
  * @param  mode: тип чтения-записи в блок памяти
  * @param  const_vol:объем блока памяти в 128-байтных кадрах
  * @param  start_frame_addr: адрес блока памяти в общем пространстве доступных кадров
  * @retval адрес начала следующего тома области в кадрах
  */
uint32_t part_const_init(type_MEM_PART_CONTROL* part_ptr, uint8_t self_num, uint8_t mode, uint16_t const_vol, uint32_t start_frame_addr)
{
  // подсчет границ и объема отдельной части
  part_ptr->start_frame_num = start_frame_addr;
  part_ptr->full_frame_num = const_vol;
  part_ptr->finish_frame_num = part_ptr->start_frame_num + part_ptr->full_frame_num - 1;
  // 
  part_ptr->write_ptr = 0;
  part_ptr->read_ptr = 0;
  part_ptr->mode = mode;
	//
	return part_ptr->finish_frame_num + 1;
}

/**
  * @brief  подсчет оставшейся памяти для отдельной части
  * @param  part_ptr: структура для управления памятью
  * @retval заполненность памяти в процентах/256
  */
uint16_t part_get_free_volume_in_percentage(type_MEM_PART_CONTROL* part_ptr)
{
  uint32_t free_frames = 0;
  if (part_ptr->write_ptr < part_ptr->read_ptr){
    free_frames = ((part_ptr->full_frame_num - part_ptr->read_ptr) + part_ptr->write_ptr);
  }
  else{
    free_frames = (part_ptr->write_ptr - part_ptr->read_ptr);
  }
  return (uint16_t)(100.*(free_frames)/part_ptr->full_frame_num);
}

/**
  * @brief  подсчет поведения указателя записи и чтения при записи
  * @param  part_ptr: указатель на структуру контроля отдельного тома памяти
  * @retval  1 - можно писать, 0 - запись заблокирована
  */
uint8_t part_wr_rd_ptr_calc(type_MEM_PART_CONTROL* part_ptr, uint8_t mode)
{
  int8_t report = 0;
  switch(part_ptr->mode){
    case PART_MODE_READ_BLOCK:  // указатель чтения догоняет указатель записи и блокируется
      if (mode == MODE_WRITE){
        part_ptr->write_ptr += 1;
        if ((part_ptr->write_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->write_ptr = 0;
        report = 1;
      }
      else if (mode == MODE_READ){
        report = 1;
        if(part_ptr->read_ptr == part_ptr->write_ptr) report = 0;
        else part_ptr->read_ptr += 1;
        if ((part_ptr->read_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->read_ptr = 0;
      }
      break;
    case PART_MODE_WRITE_BLOCK:  // указатель записи догоняет указатель чтения и блокируется
      if (mode == MODE_WRITE){
        report = 1;
        if(part_ptr->write_ptr == part_ptr->read_ptr) report = 0;
        else part_ptr->write_ptr += 1;
        if ((part_ptr->write_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->write_ptr = 0;
      }
      else if (mode == MODE_READ){
        part_ptr->read_ptr += 1;
        if ((part_ptr->read_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->read_ptr = 0;
        report = 1;
      }
      break;
    case PART_MODE_SMART_COIL_WRITE:  // указатель записи толкает указатель чтения в случае достижения оного, указатель чтения блокируется при достижении указателя записи
      if (mode == MODE_WRITE){
        part_ptr->write_ptr += 1;
        if ((part_ptr->write_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->write_ptr = 0;
        if(part_ptr->write_ptr == part_ptr->read_ptr) {
          part_ptr->read_ptr += 1;
          if ((part_ptr->read_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->read_ptr = 0;
        }
        report = 1;
      }
      else if (mode == MODE_READ){
        report = 1;
        if(part_ptr->read_ptr == part_ptr->write_ptr) report = 0;
        else part_ptr->read_ptr += 1;
        if ((part_ptr->read_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->read_ptr = 0;
      }
      break;
    case PART_MODE_REWRITE:  // указатель записи независим от указателя чтения
    default: // PART_MODE_REWRITE
      if (mode == MODE_WRITE){
        part_ptr->write_ptr += 1;
        if ((part_ptr->write_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->write_ptr = 0;
        report = 1;
      }
      else if (mode == MODE_READ){
        part_ptr->read_ptr += 1;
        if ((part_ptr->read_ptr + part_ptr->start_frame_num) > part_ptr->finish_frame_num) part_ptr->read_ptr = 0;
        report = 1;
      }
      break;
  }
  return report;
}

/**
 * @brief получение количества кадров в томе
 * 
 * @param part_ptr 
 * @return uint32_t количество кадров в томе
 */
uint32_t part_get_full_frame_count(type_MEM_PART_CONTROL* part_ptr)
{
  return part_ptr->full_frame_num;
}

/**
 * @brief получения указателя записи
 * 
 * @param part_ptr 
 * @return uint32_t указатель записи
 */
uint32_t part_get_wr_ptr(type_MEM_PART_CONTROL* part_ptr)
{
  return part_ptr->write_ptr;
}

/**
 * @brief получение указателя чтения
 * 
 * @param part_ptr 
 * @return uint32_t указатель чтения
 */
uint32_t part_get_rd_ptr(type_MEM_PART_CONTROL* part_ptr)
{
  return part_ptr->read_ptr;
}

/**
 * @brief получение отчета по тому памяти
 * 
 * @param part_ptr 
 */
type_MEM_PART_REPORT part_get_report(type_MEM_PART_CONTROL* part_ptr)
{
  part_ptr->report.rd_ptr = part_get_rd_ptr(part_ptr);
  part_ptr->report.wr_ptr = part_get_wr_ptr(part_ptr);
  part_ptr->report.full_ptr = part_get_full_frame_count(part_ptr);
  return part_ptr->report;
}

