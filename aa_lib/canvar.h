#ifndef _CANVAR_H_
#define _CANVAR_H_


#include "canv.h"



#define ERR_NO                  0
#define ERR_CANVAR_RXTIMEOUT    -20
#define ERR_CANVAR_TXTIMEOUT	  -21



void CANVar_Init(uint8_t dev_id, const typeRegistrationRec RegistrationRec[], int RegistrationRecCount);
int CANVar_Write(typeIdxMask idx, uint8_t *data, int leng);
int CANVar_Read(typeIdxMask idx, uint8_t *data, int leng);

int CANVar_Wr22(typeIdxMask idx, uint8_t *data, uint16_t leng/*<=8*/);
int CANVar_Rd22(typeIdxMask idx, uint8_t *data, uint16_t leng/*<=8*/);



#endif

