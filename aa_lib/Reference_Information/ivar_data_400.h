#ifndef _IVAR_DATA_H_
#define _IVAR_DATA_H_

#include "defs.h"
#include "ivar_handlers.h"


/*--------------------------------- TX -------------------------------------------*/

#define CMD_TX_SETPARAMS                    1
#define CMD_TX_TRANSMIT                     2
#define CMD_TX_SETPARAMS_TRANSMIT           3
#define CMD_TX_FORCE_BIAS                   9   /*сервисный режим*/


#define BW_UNDEF            0
#define BW_62_5K            6
#define BW_125K             7
#define BW_250K             8
#define BW_500K             9


#pragma pack(push, 1)

typedef struct {
  struct {
    uint8_t  BW;      // 6..9                   +0
    uint8_t  SF;      // 6..12                  +1
    uint8_t  CR;      // 1..4 : 4/5..4/8        +2
    uint8_t  CRC_;    // 0 or 1                 +3
    uint8_t  LDR;     // 0 or 1                 +4
    uint8_t  Sync;    // e.g.  0x12             +5
    uint8_t  res1[2]; //                        +6..7
    } LoraPrm;
  struct {
    uint32_t RFreq;   // 390000000..525000000   +8
    uint8_t  FPwr;    // 0..15 : min..max       +12
    } RF;
  uint8_t res2[3];    //                        +13..15
  uint8_t  PLeng;   //                          +16
  uint8_t  Payload[256];  //                    +17...
}typeIVarTxSet;


typedef struct {
  uint8_t   Cmd;       //                       +0
  int8_t  Status;      //                       +1
}typeIVarTxCtrl;


#pragma pack(pop)

/*--------------------------------- RX -------------------------------------------*/

#define CMD_RX_SETPARAMS                    1


#pragma pack(push, 1)

typedef struct {
  uint8_t  Enable;
  uint8_t  res[3];  
  int32_t  IF_freq;
}typeRxMltChanSet;    // Multi channel x8
/*size = 8 */

typedef struct {
  uint8_t  Enable;
  uint8_t  SF;          // 5..12
  uint8_t  BW;          // 7,8,9 : 125kHz,250kHz,500kHz
  uint8_t  res;
  int32_t  IF_freq;
}typeRxStdChanSet;      // Service channel
/*size = 8*/

typedef struct {
  uint8_t  Enable;
  uint8_t  LDR;         // 0, 1
  uint8_t  Sync;        // 0 - private (0x12),  1 - public (0x34), 2..127 - 0x02..0x7F
  uint8_t  res1;
  uint32_t RF_freq;     // 390000000 .. 525000000
  typeRxMltChanSet RxMltChanSet[8];
  typeRxStdChanSet RxStdChanSet;
}typeIVarRxSet;
/*size = 8 + 8*8 + 8 = 80 */

typedef struct {
  uint8_t   Cmd;       //                       +0
  int8_t  Status[4];   //                       +1
}typeIVarRxCtrl;


typedef struct {
  uint16_t    tot_size;             // total packet size 
  uint32_t    freq_hz;              // central frequency of the IF chain            +0
  int32_t     freq_offset;          //                                              +4
  uint8_t     if_chain;             // by which IF chain was packet received        +8
  uint8_t     status;               //                                              +9
  uint32_t    count_us;             // counter for timestamping                     +10
  uint8_t     dev_id;               //                                              +14
  uint8_t     modem_id;             // 0, 1, 2, 3                                   +15
  uint8_t     modulation;           //                                              +16
  uint8_t     bandwidth;            // modulation bandwidth (LoRa only)             +17
  uint8_t     datarate;             // RX datarate of the packet (SF for LoRa)      +18
  uint8_t     coderate;             // error-correcting code of the packet (LoRa only)   +19
  float       rssic;                // average RSSI of the channel in dB            +20
  float       rssis;                // average RSSI of the signal in dB             +24
  float       snr;                  // average packet SNR, in dB (LoRa only)        +28
  float       snr_min;              //                                              +32
  float       snr_max;              //                                              +36
  uint16_t    crc;                  // CRC that was received in the payload         +40
  uint16_t    size;                 // payload size in bytes                        +42
  uint8_t     payload[256];         //                                              +44
}typePktRx;


typedef struct {
  uint32_t   PktCount;
  uint32_t   InRAM_Head;
  uint32_t   InRAM_Tail;
}typeIVarRxPktInfo;


#pragma pack(pop)


/*--------------------------------- Service -------------------------------------------*/

#pragma pack(push, 1)

typedef struct {
  uint8_t  AD5592_dout_data;  //      +0
  uint8_t  AD5592_dout_mask;  //      +1
  uint16_t AD5592_dac;        //      +2
  uint16_t AD5592_adc0;       //      +4
  uint16_t AD5592_adc1;       //      +6
  uint16_t BiasDAC;           //      +8
  uint32_t SX1302_RxFreq[4];  //      +10..25      
  uint16_t RFFC0_RxLo;
}typeIVarService;

#pragma pack(pop)

/*----------------------------------------------------------------------------*/

/**** Global Variables ***/

_GLDECL_ typeIVarTxSet IVarTxSet;
_GLDECL_ typeIVarTxCtrl IVarTxCtrl;
_GLDECL_ typeIVarRxSet IVarRxSet[4];
_GLDECL_ typeIVarRxCtrl IVarRxCtrl;
_GLDECL_ typeIVarRxPktInfo IVarRxPktInfo;
_GLDECL_ typePktRx  IVarPktRx;
_GLDECL_ typeIVarService IVarService;



#ifdef _OWNER_
const char Version[128] = "Mf_400 Ver_01 2023_11_13";

const typeRegistrationRec RegistrationRec[] = {
  /*{указатель на IVar, размер IVar, указатель на Callback, id_IVar, доступ}*/
  {(void*)&Version, sizeof(Version), NULL, 1, CAN_AFLG_READONLY},
  {(void*)&IVarTxSet, sizeof(IVarTxSet), CANCallback_IVarTxSet, 2, 0},
  {(void*)&IVarTxCtrl, sizeof(IVarTxCtrl), NULL, 3, 0},
  {(void*)&IVarRxSet, sizeof(IVarRxSet), NULL, 4, 0},
  {(void*)&IVarRxCtrl, sizeof(IVarRxCtrl), NULL, 5, 0},
  {(void*)&IVarRxPktInfo, sizeof(IVarRxPktInfo), NULL, 6, CAN_AFLG_READONLY},
  {(void*)&IVarPktRx, sizeof(IVarPktRx), CANCallback_IVarPktRx, 7, CAN_AFLG_READONLY},
  {(void*)0x90000000, 0x200000, NULL, 8, CAN_AFLG_READONLY},
  {(void*)&IVarService, sizeof(IVarService), CANCallback_IVarService, 14, 0}
};

#endif



#endif



