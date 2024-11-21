#include <stm32f7xx.h>
#include <string.h>
#include "canvar.h"


#define _SYSCLK_140MHZ_


uint8_t CANMasterRx[8];  // буфер для читаемых переменных



void CANVar_Init(uint8_t dev_id, const typeRegistrationRec RegistrationRec[], int RegistrationRecCount) {
  uint8_t n;
  uint32_t btr;
  /*инициализация таймера #6 для ReadTimeout-а*/
  RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
  TIM6->CR1 = TIM_CR1_OPM;  //Timer6: one pulse mode
#ifdef _SYSCLK_140MHZ_
  TIM6->PSC = 70-1;  // 1 us prescaler
  TIM6->ARR = 500-1;  //500 us CANRx-Tx Timeout
  btr = 0x01220000 | (140000000/4/7/1000000 - 1);
#elif _SYSCLK_100MHZ_
  TIM6->PSC = 50-1;  // 1 us prescaler
  TIM6->ARR = 500-1;  //500 us CANRx-Tx Timeout
  btr = (1<<24) | ((3-1)<<20) | ((6-1)<<16) | (5 - 1);
#else
  #error "SYSCLK not defined"
#endif
  /**/
  CAN_Init(btr);
  for(n=0; n<RegistrationRecCount; n++) {
    CAN_RegisterVar(&RegistrationRec[n], n, dev_id);
    }
}



int CANVar_Write(typeIdxMask idx, uint8_t *data, int leng) {
  int err = -1;
  idx.uf.RTR = 0;
  for(;;) {
    if(leng <= 8) {
      err = CANVar_Wr22(idx, data, leng);
      break;
      }
    else {
      err = CANVar_Wr22(idx, data, 8);
      idx.uf.Offset += 8;
      data += 8;
      leng -= 8;
      }
    if(err) break;
    }
  return err;
}



int CANVar_Read(typeIdxMask idx, uint8_t *data, int leng) {
  int err = -1;
  idx.uf.RTR = 1;
  for(;;) {
    if(leng <= 8) {
      err = CANVar_Rd22(idx, data, leng);
      break;
      }
    else {
      err = CANVar_Rd22(idx, data, 8);
      idx.uf.Offset += 8;
      data += 8;
      leng -= 8;
      }
    if(err) break;
    }
  if(err)
    memset(data, 0, leng);
  return err;
}


/*-------------------------------------------------------------------------------------------*/
int CANVar_Wr22(typeIdxMask idx, uint8_t *data, uint16_t leng/*<=8*/) {
  int err;
  idx.uf.RTR = 0;
  CAN_SetFilterId(0, idx, 0);  //enable
  CANMasterWriteAckFlg = 0;
  err = CAN_Tx(idx, data, leng);
  if(err == 0) {
    /*ждем подтверждения записи*/
    TIM6->CNT = 0;
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0;
    TIM6->CR1 |= TIM_CR1_CEN;
    while(CANMasterWriteAckFlg == 0) {
      if(TIM6->SR & 1) {
        /* таймаут */
        err = ERR_CANVAR_TXTIMEOUT;
        break;
        }
      }
    }
  CAN_SetFilterId(0, idx, 1);  //disable
  return err;
}



int CANVar_Rd22(typeIdxMask idx, uint8_t *data, uint16_t leng/*<=8*/) {
  int err;
  idx.uf.RTR = 0;
  CAN_SetFilterId(0, idx, 0);  //enable
  idx.uf.RTR = 1;
  CANMasterRxFlg = 0;
  err = CAN_Tx(idx, NULL, leng);
  if(err == 0) {
    /*ждем приема*/
    TIM6->CNT = 0;
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0;
    TIM6->CR1 |= TIM_CR1_CEN;
    while(CANMasterRxFlg == 0) {
      if(TIM6->SR & 1) {
        /* таймаут */
        err = ERR_CANVAR_TXTIMEOUT;
        break;
        }
      }
    }
  if(err == 0) {
    memcpy(data, CANMasterRx, leng);
    }
  CAN_SetFilterId(0, idx, 1);  //disable
  return err;
}



