#ifndef _MIM_RS485_TR_LVL_H_
#define _MIM_RS485_TR_LVL_H_

#include <stdio.h>
#include "usart.h"
#include "my_gpio.h"
#include "crc16.h"
#include "debug.h"
#include "mim_rs485_tr_lvl_data_map.h"
#include "segmented_data.h"

/* @defgroup UART_Error_Definition   UART Error Definition
// #define  HAL_UART_ERROR_NONE             (0x00000000U)    !< No error                */
// #define  HAL_UART_ERROR_PE               (0x00000001U)    !< Parity error            */
// #define  HAL_UART_ERROR_NE               (0x00000002U)    !< Noise error             */
// #define  HAL_UART_ERROR_FE               (0x00000004U)    !< Frame error             */
// #define  HAL_UART_ERROR_ORE              (0x00000008U)    !< Overrun error           */
// #define  HAL_UART_ERROR_DMA              (0x00000010U)    !< DMA transfer error      */
// #define  HAL_UART_ERROR_RTO              (0x00000020U)    !< Receiver Timeout error  */


#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#define MIM_TR_PACKET_TIMEOUT_US        (20)

#define MIM_TR_FRAME_SELF_ADDR          (0x0C)
#define MIM_TR_FRAME_START_SYMBOL       (0xF0)
#define MIM_TR_FRAME_STOP_SYMBOL        (0x0F)

#define MIM_TR_FRAME_FLAGS_RSRV         (0x00)
#define MIM_TR_FRAME_FLAGS_CMD          (0x12)
#define MIM_TR_FRAME_FLAGS_CMD_ANSW     (0x10)
#define MIM_TR_FRAME_FLAGS_DATA         (0x04)
#define MIM_TR_FRAME_FLAGS_DATA_ANSW    (0x08)

#define MIM_TR_FRAME_ACK                (0x5A)
#define MIM_TR_FRAME_NOT_ACK            (0xA5)

#define MIM_TR_FRAME_RX_ERROR_EMPTY             (0x00)
#define MIM_TR_FRAME_RX_ERROR_START_SYMB        (0x01<<0)
#define MIM_TR_FRAME_RX_ERROR_CRC8              (0x01<<1)
#define MIM_TR_FRAME_RX_ERROR_LEN               (0x01<<2)
#define MIM_TR_FRAME_RX_ERROR_STOP_SYMB         (0x01<<3)


#define MIM_INF_LVL_FLAGS_RD (1<<0)
#define MIM_INF_LVL_FLAGS_CMD (1<<1)

// настройки RX FIFO
#define FIFO_DEPTH          (4)                      //! глубина FIFO в принятых пакетах
#define FIFO_REC_LEN        (MIM_TR_FRAME_SIZE_MAX)  //! длина единичной записи

typedef enum{
    MIM_TR_CH0, MIM_TR_CH1,
    MIM_TR_CH_NUM
} type_MIM_TR_CH;

#pragma pack(push, 1)

typedef union {
    uint8_t array[MIM_TR_FRAME_SIZE_MAX];
    struct {
        uint8_t start_symb;
        uint8_t rx_addr;    //адрес получателя
        uint8_t tx_addr;    //адрес отправителя
        uint8_t flags;
        uint16_t data_len;
        union{
            uint8_t raw_data[MIM_TR_DATA_SIZE_MAX];
            struct{
                uint8_t id;
                uint8_t data[MIM_TR_DATA_SIZE_MAX - 1];
            } fields;
        }inf;
        uint8_t crc8;
        uint8_t stop_symb;
    } fields;
} type_MIM_TR_FRAME;

#pragma pack(pop)

#pragma pack(push, 2)

/** 
  * @brief  структура-интерфейс к UART-периферии поду правлением HAL
  */
typedef struct
{
    UART_HandleTypeDef* huart;
    TIM_HandleTypeDef* tim;
    type_GPIO_setting ch0_ena, ch1_ena;
    //
    type_MIM_INF_ID_DATA* data_map;
    //
    type_MIM_TR_FRAME rx_frame, tx_frame;
    uint8_t tx_data[MIM_TR_FRAME_SIZE_MAX];
    uint16_t tx_data_size;
    uint8_t tx_addr;
    uint8_t used_rx_ch_de;
    uint8_t de_invert;
    uint8_t channel_ena; // отслеживание работоспособности канала связи
    IRQn_Type huart_irq;
    //
    uint8_t rx_buffer[MIM_TR_FRAME_SIZE_MAX];
    uint16_t rx_ptr;
    uint8_t rx_ready;
    uint32_t rx_packet_num;
    uint8_t rx_crc8;
    uint8_t rx_packet_error;
    uint16_t rx_data_len;
    uint16_t rx_id;
    //
    type_SEGM segm;
    //
    uint8_t error_status;
    uint32_t error_cnter;
    //
} type_MIM_TR_LVL_Interface;

#pragma pack(pop)

void mim_tr_init(type_MIM_TR_LVL_Interface* mim_tr_ptr, UART_HandleTypeDef* huart, IRQn_Type huart_irq, GPIO_TypeDef* ch0_ena_bank, uint16_t ch0_ena_line, GPIO_TypeDef* ch1_ena_bank, uint16_t ch1_ena_line, TIM_HandleTypeDef* tim, type_MIM_INF_ID_DATA* data_map, uint8_t de_invert);
uint16_t mim_tr_form_tx_data(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_tx_frame_fill(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t flag, uint8_t id, uint8_t* data, uint16_t data_len);
void mim_tr_set_de(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t val);
int8_t mim_tr_transmit_tx_frame(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_it_tx_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_it_rx_timer_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_cmd_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id, uint8_t* rx_data, uint16_t rx_data_len);
void mim_tr_cmd_with_segm_data_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id, uint8_t* rx_data, uint16_t rx_data_len);
int8_t mim_tr_redirect_segm_data_cmd(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_rd_data_proccess(type_MIM_TR_LVL_Interface* mim_tr_ptr, uint8_t rx_id);
void mim_tr_it_rx_uart_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr);
uint8_t mim_tr_it_check_rx_packet(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_it_rx_error_cb(type_MIM_TR_LVL_Interface* mim_tr_ptr);
int16_t mim_tr_get_processed_cmd_id(type_MIM_TR_LVL_Interface* mim_tr_ptr);
int16_t mim_tr_get_processed_data_id(type_MIM_TR_LVL_Interface* mim_tr_ptr);

void mim_tr_inh_communication(type_MIM_TR_LVL_Interface* mim_tr_ptr);
void mim_tr_ena_communication(type_MIM_TR_LVL_Interface* mim_tr_ptr);


#endif
