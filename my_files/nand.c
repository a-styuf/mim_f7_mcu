  /**
 * @file nand_flash.c
 * @author Alexey Styuf (a-styuf@yandex.ru)
 * @brief 
 * @version 0.1
 * @date 2024-09-23
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "nand.h"

void nand_init(type_NAND *nand_ptr)
{
  nand_reset_parameters(nand_ptr);
  nand_ptr->hnand = &hnand1;
  //
  
  //
  nand_ptr->frame_size_b = NAND_FRAME_SIZE;
  nand_ptr->page_size_b = nand_ptr->hnand->Config.PageSize;
  nand_ptr->page_size_fr =  nand_ptr->page_size_b / nand_ptr->frame_size_b;
	
	nand_ptr->block_size_page = nand_ptr->hnand->Config.BlockSize;
  nand_ptr->block_size_b = nand_ptr->block_size_page * nand_ptr->page_size_b;
  nand_ptr->block_size_fr = nand_ptr->block_size_page * nand_ptr->page_size_fr;
          
  nand_ptr->block_num_full = nand_ptr->hnand->Config.BlockNbr;
  nand_ptr->page_num_full = nand_ptr->hnand->Config.BlockNbr * nand_ptr->hnand->Config.BlockSize;
  nand_ptr->frame_num_full = nand_ptr->block_size_fr * nand_ptr->block_size_fr;
  //
  nand_hal_reset_nand(nand_ptr);

  //
  nand_hal_read_id(nand_ptr);
  // // nand test preparation
  // //
  // nand_hal_erase(nand_ptr, 0);
  // // Test write
  // memset(nand_ptr->wr_frame, 0xF0, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 0, nand_ptr->wr_frame);
  // memset(nand_ptr->wr_frame, 0xF1, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 1, nand_ptr->wr_frame);
  // memset(nand_ptr->wr_frame, 0xF2, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 2, nand_ptr->wr_frame);
  // memset(nand_ptr->wr_frame, 0xF3, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 3, nand_ptr->wr_frame);
  // memset(nand_ptr->wr_frame, 0xF4, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 4, nand_ptr->wr_frame);
  // memset(nand_ptr->wr_frame, 0xF5, sizeof(nand_ptr->wr_frame));
  // nand_write_frame(nand_ptr, 0, 5, nand_ptr->wr_frame);
  // // Test read
  // nand_read_frame(nand_ptr, 0, 0, nand_ptr->rd_frame);
  // nand_read_frame(nand_ptr, 0, 1, nand_ptr->rd_frame);
  // nand_read_frame(nand_ptr, 0, 2, nand_ptr->rd_frame);
  // nand_read_frame(nand_ptr, 0, 3, nand_ptr->rd_frame);
  // nand_read_frame(nand_ptr, 0, 4, nand_ptr->rd_frame);
  // nand_read_frame(nand_ptr, 0, 5, nand_ptr->rd_frame);
  //
  printf("%s: NAND init 0x%02X %02X %02X %02X %02X %02X (0xECD6xxxxxxxx - is OK)\n", now(), 
                                                                                      nand_ptr->id.array[0], 
                                                                                      nand_ptr->id.array[1], 
                                                                                      nand_ptr->id.array[2], 
                                                                                      nand_ptr->id.array[3], 
                                                                                      nand_ptr->id.array[4], 
                                                                                      nand_ptr->id.array[5]);
}

void nand_reset_parameters(type_NAND *nand_ptr)
{
  memset((uint8_t*)nand_ptr, 0x00, sizeof(type_NAND));
}

int8_t nand_hal_reset_nand(type_NAND *nand_ptr)
{
  NAND_HandleTypeDef *hnand = &hnand1;
  uint32_t deviceaddress;
  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY){
    /* Process Locked */
    __HAL_LOCK(hnand);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;
    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
    /* Send NAND reset command */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = 0xFF;
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;
    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else {
    return HAL_ERROR;
  }
  return HAL_OK;
}

int8_t nand_hal_read_id(type_NAND *nand_ptr)
{
  __IO uint32_t data = 0;
  __IO uint32_t data1 = 0;
  uint32_t deviceaddress;
  NAND_HandleTypeDef *hnand = &hnand1;
  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY) {
    /* Process Locked */
    __HAL_LOCK(hnand);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;
    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
    /* Send Read ID command sequence */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA))  = NAND_CMD_READID;
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = 0x00;
    __DSB();
    /* Read the electronic signature from NAND flash */
    if (hnand->Init.MemoryDataWidth == FMC_NAND_PCC_MEM_BUS_WIDTH_8) {
      /* Return the data read */
      nand_ptr->id.f.marker_code  = *(__IO uint8_t *)deviceaddress + 0;
      nand_ptr->id.f.device_code  = *(__IO uint8_t *)deviceaddress + 1;
			nand_ptr->id.f.int_info  		= *(__IO uint8_t *)deviceaddress + 2;
			nand_ptr->id.f.page_info  	= *(__IO uint8_t *)deviceaddress + 3;
			nand_ptr->id.f.fifth_code  	= *(__IO uint8_t *)deviceaddress + 4;
			nand_ptr->id.f.sixth_code  	= *(__IO uint8_t *)deviceaddress + 5;
    }
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;
    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else {
    return HAL_ERROR;
  }
  return HAL_OK;
}

int8_t nand_write_frame(type_NAND *nand_ptr, uint32_t page_num, uint8_t frame_num, uint8_t *frame)
{
  uint32_t index;
  uint32_t tickstart;
  uint32_t deviceaddress;
  uint32_t nandaddress;
  uint32_t frame_addr; //! адрес кадра внутри страницы
  uint8_t *buff = frame;
  NAND_HandleTypeDef *hnand = &hnand1;
  NAND_AddressTypeDef Address;

  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY) {
    /* Process Locked */
    __HAL_LOCK(hnand);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;
    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
    /* NAND raw address calculation */
    Address.Block = page_num / hnand->Config.BlockSize;
    Address.Page = page_num % hnand->Config.BlockSize;
    Address.Plane = 0;
    nandaddress = ARRAY_ADDRESS(&Address, hnand);
    frame_addr = frame_num * nand_ptr->frame_size_b; //TODO: сделать проверку на превышение
    if ((frame_addr + nand_ptr->frame_size_b) > nand_ptr->page_size_b){
      frame_addr = (nand_ptr->page_size_b / nand_ptr->frame_size_b) - 1;  //костыльная защита от слишком большого номера блока для записи
    }
    /* Send write page command sequence */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_AREA_A;
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_WRITE0;
    __DSB();

    /* Cards with page size <= 512 bytes */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(frame_addr);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(frame_addr);
    __DSB();
    //
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_3RD_CYCLE(nandaddress);
    __DSB();

    /* Write data to memory */
    for (index = 0U; index < nand_ptr->frame_size_b; index++) {
      *(__IO uint8_t *)deviceaddress = *buff;
      buff++;
      __DSB();
    }

    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_WRITE_TRUE1;
    __DSB();

    /* Get tick */
    tickstart = HAL_GetTick();
    /* Read status until NAND is ready */
    while (nand_hal_read_status(nand_ptr) != NAND_READY) {
      if ((HAL_GetTick() - tickstart) > NAND_WRITE_TIMEOUT) {
        /* Update the NAND controller state */
        hnand->State = HAL_NAND_STATE_ERROR;
        /* Process unlocked */
        __HAL_UNLOCK(hnand);
        return HAL_TIMEOUT;
      }
    }
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;
    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else{
    return HAL_ERROR;
  }
  return HAL_OK;
}

int8_t nand_write_page(type_NAND *nand_ptr, uint32_t page_num, uint8_t *page)
{
  uint32_t index;
  uint32_t tickstart;
  uint32_t deviceaddress;
  uint32_t nandaddress;
  uint32_t blockaddress; //! адрес кадра внутри страницы
  uint8_t *buff = page;
  NAND_HandleTypeDef *hnand = &hnand1;
  NAND_AddressTypeDef Address;

  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY) {
    /* Process Locked */
    __HAL_LOCK(hnand);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;
    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
    /* NAND raw address calculation */
    Address.Block = page_num / nand_ptr->block_size_page;
    Address.Page = page_num % nand_ptr->block_size_page;
    Address.Plane = 0;
    nandaddress = ARRAY_ADDRESS(&Address, hnand);
    blockaddress = 0;
    /* Send write page command sequence */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_AREA_A;
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_WRITE0;
    __DSB();

    /* Cards with page size <= 512 bytes */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(blockaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(blockaddress);
    __DSB();
    //
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_3RD_CYCLE(nandaddress);
    __DSB();

    /* Write data to memory */
    for (index = 0U; index < nand_ptr->page_size_b; index++) {
      *(__IO uint8_t *)deviceaddress = *buff;
      buff++;
      __DSB();
    }

    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_WRITE_TRUE1;
    __DSB();

    /* Get tick */
    tickstart = HAL_GetTick();
    /* Read status until NAND is ready */
    while (nand_hal_read_status(nand_ptr) != NAND_READY) {
      if ((HAL_GetTick() - tickstart) > NAND_WRITE_TIMEOUT) {
        /* Update the NAND controller state */
        hnand->State = HAL_NAND_STATE_ERROR;
        /* Process unlocked */
        __HAL_UNLOCK(hnand);
        return HAL_TIMEOUT;
      }
    }
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;
    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else{
    return HAL_ERROR;
  }
  return HAL_OK;
}

int8_t nand_read_frame(type_NAND *nand_ptr, uint32_t page_num, uint8_t frame_num, uint8_t *frame)
{
  uint32_t index;
  uint32_t tickstart;
  uint32_t deviceaddress;
  uint32_t nandaddress;
  uint32_t frame_addr;
  // uint8_t *buff = frame;
	NAND_HandleTypeDef *hnand = &hnand1;
  NAND_AddressTypeDef Address;
  //
  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY) {
    /* Process Locked */
    __HAL_LOCK(hnand);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;
    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
    /* NAND raw address calculation */
    Address.Block = page_num / nand_ptr->block_size_page;
    Address.Page = page_num % nand_ptr->block_size_page;
    Address.Plane = 0;
    nandaddress = ARRAY_ADDRESS(&Address, hnand);
    frame_addr = frame_num * nand_ptr->frame_size_b; //TODO: сделать проверку на превышение
    if ((frame_addr + nand_ptr->frame_size_b) > nand_ptr->page_size_b){
      frame_addr = (nand_ptr->page_size_b / nand_ptr->frame_size_b) - 1;  //костыльная защита от слишком большого номера блока для записи
    }
    /* Send read page command sequence */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_AREA_A;
    __DSB();

    /* Cards with page size <= 512 bytes */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(frame_addr);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(frame_addr);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_3RD_CYCLE(nandaddress);
    __DSB();

    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA))  = NAND_CMD_AREA_TRUE1;
    __DSB();

    if (hnand->Config.ExtraCommandEnable == ENABLE) {
      /* Get tick */
      tickstart = HAL_GetTick();
      /* Read status until NAND is ready */
      while (nand_hal_read_status(nand_ptr) != NAND_READY) {
        if ((HAL_GetTick() - tickstart) > NAND_WRITE_TIMEOUT) {
          /* Update the NAND controller state */
          hnand->State = HAL_NAND_STATE_ERROR;
          /* Process unlocked */
          __HAL_UNLOCK(hnand);
          return HAL_TIMEOUT;
        }
      }
      /* Go back to read mode */
      *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = ((uint8_t)0x00);
      __DSB();
    }

    /* Get Data into Buffer */
    for (index = 0U; index < nand_ptr->frame_size_b; index++) {
      //*buff = *(uint8_t *)deviceaddress;
      nand_ptr->rd_frame[index] = *(uint8_t *)deviceaddress;
      //buff++;
    }
    //
    memcpy(frame, (uint8_t*)nand_ptr->rd_frame, nand_ptr->frame_size_b);
    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;
    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else {
    return HAL_ERROR;
  }
  return HAL_OK;
}

uint32_t nand_hal_read_status(type_NAND *nand_ptr)
{
  uint32_t data;
  uint32_t deviceaddress;
  NAND_HandleTypeDef *hnand = &hnand1;
  UNUSED(hnand);

  /* Identify the device address */
  deviceaddress = NAND_BANK_ADDR;

  /* Send Read status operation command */
  *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_STATUS;

  /* Read status register data */
  data = *(__IO uint8_t *)deviceaddress;

  /* Return the status */
  if ((data & NAND_ERROR) == NAND_ERROR)
  {
    return NAND_ERROR;
  }
  else if ((data & NAND_READY) == NAND_READY)
  {
    return NAND_READY;
  }
  else
  {
    return NAND_BUSY;
  }
}

int8_t nand_hal_erase(type_NAND *nand_ptr, uint32_t block_num)
{
  uint32_t deviceaddress;
  uint32_t nandaddress;
  NAND_HandleTypeDef *hnand = &hnand1;
  NAND_AddressTypeDef Address;

  /* Check the NAND controller state */
  if (hnand->State == HAL_NAND_STATE_BUSY) {
    return HAL_BUSY;
  }
  else if (hnand->State == HAL_NAND_STATE_READY) {
    /* Process Locked */
    __HAL_LOCK(hnand);

    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_BUSY;

    /* Identify the device address */
    deviceaddress = NAND_BANK_ADDR;
		Address.Block = block_num;
		Address.Page = 0;
		Address.Plane = 0;
		nandaddress = ARRAY_ADDRESS(&Address, hnand);
		
		/* Send Erase block command sequence */
    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_ERASE0;
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_1ST_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_2ND_CYCLE(nandaddress);
    __DSB();
    *(__IO uint8_t *)((uint32_t)(deviceaddress | ADDR_AREA)) = ADDR_3RD_CYCLE(nandaddress);
    __DSB();

    *(__IO uint8_t *)((uint32_t)(deviceaddress | CMD_AREA)) = NAND_CMD_ERASE1;
    __DSB();

    /* Update the NAND controller state */
    hnand->State = HAL_NAND_STATE_READY;

    /* Process unlocked */
    __HAL_UNLOCK(hnand);
  }
  else {
    return HAL_ERROR;
  }
  return HAL_OK;
}

int8_t nand_lin_write_block_set(type_NAND *nand_ptr, uint32_t block_num)
{
  nand_ptr->wr_frame_ptr = 0;
  nand_ptr->wr_frame_cnt = 0;
  nand_ptr->wr_page_ptr = 0;
  nand_ptr->line_write_error = 0;
  if(block_num < nand_ptr->block_num_full)
  {
    nand_ptr->wr_current_block = block_num;
    nand_ptr->wr_start_block = block_num;
    nand_hal_erase(nand_ptr, nand_ptr->wr_current_block);
    return 0;
  }
  else{
    nand_ptr->wr_current_block = 0;
    nand_ptr->wr_start_block = 0;
    nand_ptr->line_write_error++;
    return -1;
  }
}

int8_t nand_lin_write_frame(type_NAND *nand_ptr, uint8_t *frame)
{
  if(nand_ptr->line_write_error){
    return -1;
  }
  else{
    if(nand_ptr->wr_frame_ptr == 0){
      memset((uint8_t*)nand_ptr->page_buffer, 0x00, sizeof(nand_ptr->page_buffer));
      memcpy((uint8_t*)nand_ptr->page_buffer + 128*nand_ptr->wr_frame_ptr, frame, nand_ptr->frame_size_b);
      nand_ptr->wr_frame_cnt++;
      nand_ptr->wr_frame_ptr++;
    }
    else if(nand_ptr->wr_frame_ptr >= ((nand_ptr->page_size_b/nand_ptr->frame_size_b) - 1)){
      // пишем последний кадр в страницу и записываем страницу в память
      memcpy((uint8_t*)nand_ptr->page_buffer + 128*nand_ptr->wr_frame_ptr, frame, nand_ptr->frame_size_b);
      nand_write_page(nand_ptr, nand_ptr->wr_page_ptr + nand_ptr->wr_current_block*(nand_ptr->block_size_page), nand_ptr->page_buffer);
      // проверяем на последнюю страницу в блоке
      if(nand_ptr->wr_page_ptr >= (nand_ptr->block_size_page - 1)){
        nand_ptr->wr_page_ptr = 0;
        nand_ptr->wr_current_block++;
        if (nand_ptr->wr_current_block < nand_ptr->block_num_full - 1){
          nand_hal_erase(nand_ptr, nand_ptr->wr_current_block);
        }
        else{
          nand_ptr->line_write_error ++;
        }
      }
      else{
        nand_ptr->wr_page_ptr++;
      }
      //
      nand_ptr->wr_frame_ptr = 0;
      nand_ptr->wr_frame_cnt++;
    }
    else{
      memcpy((uint8_t*)nand_ptr->page_buffer + 128*nand_ptr->wr_frame_ptr, frame, nand_ptr->frame_size_b);
      nand_ptr->wr_frame_ptr++;
      nand_ptr->wr_frame_cnt++;
    }
  }
return 0;
}

int8_t nand_lin_write_finish(type_NAND *nand_ptr)
{
  if(nand_ptr->line_write_error){
    return -1;
  }
  else{
      if(nand_ptr->wr_frame_ptr != 0){
        nand_write_page(nand_ptr, 
                        nand_ptr->wr_page_ptr + nand_ptr->wr_current_block*(nand_ptr->block_size_page), 
                        nand_ptr->page_buffer);
      }
  }
  return 0;
}

int8_t nand_lin_read_block_set(type_NAND *nand_ptr, uint32_t block_num)
{
  nand_ptr->rd_frame_ptr = 0;
  nand_ptr->rd_frame_cnt = 0;
  nand_ptr->rd_page_ptr = 0;
  nand_ptr->line_read_error = 0;
  if(block_num < nand_ptr->block_num_full){
    nand_ptr->rd_current_block = block_num;
    nand_ptr->rd_start_block = block_num;
    return 0;
  }
  else{
    nand_ptr->rd_current_block = 0;
    nand_ptr->rd_start_block = 0;
    nand_ptr->line_read_error++;
    return -1;
  }
}

int8_t nand_lin_read_frame(type_NAND *nand_ptr, uint8_t *frame)
{
  if(nand_ptr->line_read_error){
    return -1;
  }
  else{
    if(nand_ptr->rd_frame_ptr == 0){
      nand_read_frame(nand_ptr, 
                      nand_ptr->block_size_page*nand_ptr->rd_current_block + nand_ptr->rd_page_ptr,
                      nand_ptr->rd_frame_ptr, 
                      frame);
      nand_ptr->rd_frame_ptr++;
      nand_ptr->rd_frame_cnt++;
    }
    else if(nand_ptr->rd_frame_ptr >= ((nand_ptr->page_size_b/nand_ptr->frame_size_b) - 1)){
      nand_read_frame(nand_ptr, 
                nand_ptr->block_size_page*nand_ptr->rd_current_block + nand_ptr->rd_page_ptr,
                nand_ptr->rd_frame_ptr, 
                frame);
      if(nand_ptr->rd_page_ptr >= (nand_ptr->block_size_page - 1)){
        nand_ptr->rd_page_ptr = 0;
        nand_ptr->rd_current_block++;
        if (nand_ptr->rd_current_block < nand_ptr->block_num_full - 1){
          //
        }
        else{
          nand_ptr->line_read_error++;
        }
      }
      else{
        nand_ptr->rd_page_ptr++;
      }
      //
      nand_ptr->rd_frame_ptr = 0;
      nand_ptr->rd_frame_cnt++;
    }
    else{
      nand_read_frame(nand_ptr, 
                nand_ptr->block_size_page*nand_ptr->rd_current_block + nand_ptr->rd_page_ptr,
                nand_ptr->rd_frame_ptr, 
                frame);
      nand_ptr->rd_frame_ptr++;
      nand_ptr->rd_frame_cnt++;
    }
  }
  return 0;
}

