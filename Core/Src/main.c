/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "debug.h"
#include "clock.h"
#include "sys_uart.h"
#include "pwr.h"
#include "mim.h"
#include "radio.h"
#include "task_planner.h"
#include "mim_rs485_tr_lvl_data_map.h"
#include "canvar.h"
#include "nand.h"
#include "experiment.h"
#include "ext_mem.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

uint8_t* dev_id_ptr = (uint8_t*)0x8000024;
char *raw_ver = (char*)0x8000025;
char version[16] = {0};
uint8_t dev_id = 0;
volatile uint8_t systick_counter = 0;

// #define SOFT_VERSION 	 ../aa_lib/fw_ver.inc
#define IOT_FABR_NUM    (4)       //! 0 - отработочный

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typeTPStruct tp;		//! обязательный объект планировщика задач
type_MIM_DEVICE mim;
type_MIM_INF_ID_DATA mim_data;
type_SysUART suart;
type_PWR_CONTROL pwr;
type_NAND nand;
type_EXPERIMENT_CORE ex;
//
extern typeRegistrationRec LocRegistrationRec[28];
extern uint8_t CANMasterRx[8];
//
type_RADIO radio[MIM_MODULE_NUM - 1]; //TODO: проверять количество радиомодудей, если появится больше, чем один не радиомодуль
typeFT ft[FT_NUM]; //WARNING: необходимо проверять число используемых полетных заданий, согласовывать их с используемой памятью

uint8_t uint8_array[1024]={0};
int16_t int16_val = 0;
uint16_t uint16_val = 0, uint16_val_2 = 0;
int32_t id_val = 0;
uint8_t uint8_val = 0, cfg[16] = {0}, ft_cnt=0;
typeFTArray task_var;
typeSingleFrameStruct frame_1, frame_2;
//
type_MIM_INF_ID_TX_DATA_WR tx_frame;
type_MIM_INF_ID_TX_CFG tx_cfg;
type_MIM_INF_ID_CFG_REQ cfg_req;
type_MIM_INF_ID_RX_CFG rx_cfg;
type_MIM_CMD_STRUCT cmd_stuct;
type_MIM_EXP_CMD_STRUCT exp_cmd_stuct;
type_MIM_INF_ID_RX_DATA rx_packet;
type_MEM_CONTROL mem;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t i=0;
  uint8_t num = 0;
  uint8_t cmd_num = 0;
  int8_t status = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_CAN2_Init();
  MX_FMC_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM7_Init();
  MX_TIM9_Init();
  MX_UART5_Init();
  MX_UART8_Init();
  MX_UART7_Init();
  MX_UART4_Init();
  MX_USART2_UART_Init();
  MX_SPI2_Init();
  MX_USART3_UART_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  __HAL_DBGMCU_FREEZE_IWDG();
  //
	HAL_Delay(1);
  sprintf(version, "%c.%c.%c", raw_ver[0], raw_ver[1], raw_ver[2]);
	dev_id = *dev_id_ptr - '0';
	//
	
  printf("\nSW v%s\n", version);
	printf("\n______ \\_(o_O)_/ ______\n");
  clock_init();
	printf("%s: Clock init\n", now());
	HAL_Delay(100);
  printf("%s: Clock after 100 ms delay\n", now());
	printf("%s: Marafon Interface Module (MIM): dev_id <%d>, version <%s> \n", now(), dev_id, version);
  // flight task init
  ft_cnt = 0; uint8_val = 0;
  for (i = 0; i < FT_NUM; i++) {
    ft_cnt += ft_init(&ft[i], i, 0x00, 0x00);
  }
  printf("%s: Flight task init: <%d> (%d - OK), loaded <%d>\n", now(), ft_cnt, (int)FT_NUM, uint8_val);
  // main module init
  suart_init(&suart, &huart1, &htim9);
  pwr_init(&pwr, &suart);
  // инициализация памяти
  ext_mem_init(&mem, &hspi2, IOT_FABR_NUM, 0x0F);
  ext_mem_get_cfg_frame(&mem, EXT_MEM_SELF_CFG_FRAME_1_NUM, &frame_1);
  ext_mem_part_apply_cfg(&mem, frame_1);
  // инициализация NAND
  nand_init(&nand);
  ex_init(&ex, &nand, MIM, IOT_FABR_NUM, dev_id);
  mim_init(&mim, MIM, version, &pwr, IOT_FABR_NUM, dev_id, radio, &ex);
	printf("%s: Flight task init: <%d> (%d - OK), loaded <%d>\n", now(), ft_cnt, (int)FT_NUM, uint8_val);
  mim_tr_inf_init(&mim_data);
  mim_tr_init(&mim.tr_lvl[0], &huart7, UART7_IRQn, GPIOA, 11, GPIOA, 11, &htim4, &mim_data, 1);  // DE - инвертировано
  mim_tr_init(&mim.tr_lvl[1], &huart8, UART8_IRQn, GPIOB, 9, GPIOB, 9, &htim3, &mim_data, 0);  // DE - не инвертировано
  // mim_tr_init(&mim.tr_lvl[2], &huart4, UART4_IRQn, GPIOA, 12, GPIOA, 12, &htim5, &mim_data, 1);  // DE - инвертировано
  // mim_tr_init(&mim.tr_lvl[3], &huart5, UART5_IRQn, GPIOD, 3, GPIOD, 3, &htim2, &mim_data, 0);    // DE - не инвертировано //TODO: Добавить обработку остальных каналов UART
  //
  LocRegistrationRec[0].VarPtr = &CANMasterRx;
  LocRegistrationRec[0].VarLeng = sizeof(CANMasterRx);
  LocRegistrationRec[0].CallBackProc = NULL;
  LocRegistrationRec[0].ivar_id = 0;
  LocRegistrationRec[0].access_flgs =  CAN_AFLG_NOOFFSET | CAN_AFLG_MASTER | CAN_AFLG_NOACK;

  CANVar_Init(MIM + 1, LocRegistrationRec, 28);
  //
  for(uint8_t dev = 1; dev<MIM_MODULE_NUM; dev++){
    radio_init(&radio[dev-1], dev, &pwr.ch[dev], dev+1);
  }
  // test_init temporary
  //
  tp_init(&tp);
  // заполнение процессов
  tp_process_registration(&tp, &mim_process_tp, &mim, 0, TP_SHARED_MEM_VOL_B);
  for(uint8_t radio_num=1; radio_num < MIM_MODULE_NUM; radio_num++){
    tp_process_registration(&tp, &radio_process_tp, &radio[radio_num-1], radio_num*512, 512);
  }
  tp_process_registration(&tp, &pwr_process_tp, &pwr, 0, 0);
  //
  HAL_TIM_Base_Start_IT(&htim7); //task_planner
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		HAL_GPIO_WritePin(GPIOC, 1<<15, GPIO_PIN_SET);
    tp_handler(&tp);
		HAL_GPIO_WritePin(GPIOC, 1<<15, GPIO_PIN_RESET);
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    for (num = 0; num < 2; num++){
      id_val = mim_tr_get_processed_cmd_id(&mim.tr_lvl[num]);
			cmd_num = mim.tr_lvl[num].data_map->id_map[id_val];
      if (id_val >= 0){
        HAL_GPIO_WritePin(GPIOC, 1<<13, GPIO_PIN_SET);
        //
        switch (id_val){
          case MIM_INF_ID_SET_TIME:
            clock_set_time_s_and_ms(*(uint32_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[0], 
                                    *(uint16_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[4]);
            printf("%s: MIM cmd %d set_time: sec %08X part %02X\n", now(), (int)id_val,
                                                              *(uint32_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[0], 
                                                              *(uint16_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[4]);
            break;
          case MIM_INF_ID_SET_SETTINGS:
            memcpy((uint8_t*)cmd_stuct.array, (uint8_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[0], 16);
            if(cmd_stuct.fl.label == 0xABBC){
              switch(cmd_stuct.fl.cmd_num){
                case MIM_CMD_MODE:
                  switch(*(uint16_t*)&cmd_stuct.fl.data[0])
                  {
                    case MIM_MODE_TRANSCEIVER:
                      mim.ctrl.mode = MIM_MODE_TRANSCEIVER;
                      mim_set_interval_value(&mim, MIM_INTERVAL_EXPERIMENT, 1.0);
                      break;
                    case MIM_MODE_EXPERIMENT:
                      mim.ctrl.mode = MIM_MODE_EXPERIMENT;
                      break;
                  }
                  break;
                case MIM_CMD_PWR_STATE:
                  mim.pwr_ptr->ch_state = *(uint16_t*)&cmd_stuct.fl.data[0];
                  break;
                case MIM_CMD_SET_DEFAULT:
                  if((*(uint16_t*)&cmd_stuct.fl.data[0] >= 1) && (*(uint16_t*)&cmd_stuct.fl.data[0] < MIM_MODULE_NUM)){
                    radio_rx_send_cmd(&radio[*(uint16_t*)&cmd_stuct.fl.data[0] - 1], CMD_RX_SETPARAMS);
                  }
                  else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
                  break;
                case MIM_CMD_RESET_STATE:
                  mim_reset_state(&mim);
                  for (uint8_t module_num = 1; module_num < MIM_MODULE_NUM; module_num++){
                    radio_status_reset(&radio[module_num - 1]);
                  }
                  break;
              }
            }
            else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            printf("%s: MIM cmd <%d> set settings: label <0x%04X> num <%d>\n", now(),  id_val, 
                                                                            cmd_stuct.fl.label, 
                                                                            cmd_stuct.fl.cmd_num);
            break;
          //
          case MIM_INF_ID_SET_EXP_SETTINGS:
            memcpy((uint8_t*)exp_cmd_stuct.array, (uint8_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[0], sizeof(type_MIM_EXP_CMD_STRUCT));
            //
            mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_READ_EXP_SETTINGS, (uint8_t*)&mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array[0], sizeof(type_MIM_EXP_CMD_STRUCT));
            //
            if(exp_cmd_stuct.fl.label == 0xABCD){
              switch(exp_cmd_stuct.fl.cmd_num){
                case MIM_EXCMD_SETTINGS:
                  memcpy((uint8_t*)&ex.shadow_param, (uint8_t*)exp_cmd_stuct.fl.data, sizeof(type_EX_Param));
                  break;
                case MIM_EXCMD_SET_TX_DATA:
                  ex_set_tx_msg(&ex, exp_cmd_stuct.fl.data[0], (uint8_t*)&exp_cmd_stuct.fl.data[1]);
                  break;
                case MIM_EXCMD_GPOSITION:
                  memcpy((uint8_t*)ex.position_data, (uint8_t*)exp_cmd_stuct.fl.data, MIN(sizeof(ex.position_data), sizeof(ex.position_data)));
                  break;
                case MIM_EXCMD_CLEAR_MSG_BUFFER:
                  memset((uint8_t*)&ex.tx_msg_buffer, 0x00, sizeof(type_RADIO_EX_TX_MSG_BUFFER));
                  break;
              }
            }
            else mim_error_collector(&mim, MIM_ERROR_EXCMD_PARAM, MIM_ERROR_TYPE_SET);
            printf("%s: MIM excmd <%d> set settings: label <0x%04X> num <%d>\n", now(),  id_val, 
                                                                            exp_cmd_stuct.fl.label, 
                                                                            exp_cmd_stuct.fl.cmd_num);
            break;
          //
          case MIM_INF_ID_TX_CFG_REQ:
          case MIM_INF_ID_S_TX_CFG_REQ:
            if(mim.ctrl.mode == MIM_MODE_TRANSCEIVER){
              memcpy((uint8_t*)&cfg_req, mim.tr_lvl[num].data_map->unit[cmd_num].data.array, sizeof(type_MIM_INF_ID_CFG_REQ));
              printf("%s: MIM cmd <%d> tx cfg resp: module <%d>\n", now(), id_val, cfg_req.fl.module);
              if ((cfg_req.fl.packet_type == 0x01) || (cfg_req.fl.packet_type == 0x21)) {
                if ((cfg_req.fl.module < MIM_MODULE_NUM) && (cfg_req.fl.module >= 1)){
                  if (radio_tx_get_settings(&radio[cfg_req.fl.module - 1]) == 0){
                    tx_cfg.fl.id = id_val + 1;
                    tx_cfg.fl.packet_type = (cfg_req.fl.packet_type == 0x01) ? 0x02 : 0x22;
                    tx_cfg.fl.packet_size = 31;
                    tx_cfg.fl.module = cfg_req.fl.module;
                    memset(tx_cfg.fl.reserve, 0, sizeof(tx_cfg.fl.reserve));
                    tx_cfg.fl.tx_frame = radio[cfg_req.fl.module - 1].tx_ctrl.IVarTxSet_rd;
                    mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, id_val+1, tx_cfg.tr_lvl.array, 31);
                  }
                }
                else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
              }
              else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            }
            else{mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);};
            break;
          case MIM_INF_ID_TX_DATA_WR:
          case MIM_INF_ID_S_TX_DATA_WR:
            if(mim.ctrl.mode == MIM_MODE_TRANSCEIVER){
              memcpy((uint8_t*)&tx_frame, mim.tr_lvl[num].data_map->unit[cmd_num].data.array, sizeof(type_MIM_INF_ID_TX_DATA_WR));
              if((tx_frame.fl.packet_type == 0x03) || (tx_frame.fl.packet_type == 0x23)) {
                if ((tx_frame.fl.module >= 1) && (tx_frame.fl.module < MIM_MODULE_NUM)){
                  status = radio_write_tx_fifo(&radio[tx_frame.fl.module - 1], &tx_frame.fl.tx_frame);
                  if (status > 0) { mim.tx_total_cnt++; }
                  else if (status < 0) { mim.tx_lost_cnt++; }
                  else {}
                }
                else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
              }
              else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
              printf("%s: MIM cmd %d send data to ASP: module %04X\n", now(), id_val, tx_frame.fl.module);
            }
            else{mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);};
            break;
          case MIM_INF_ID_RX_CFG_REQ:
            memcpy((uint8_t*)&cfg_req, mim.tr_lvl[num].data_map->unit[cmd_num].data.array, sizeof(type_MIM_INF_ID_CFG_REQ));
            printf("%s: MIM cmd <%d> rx cfg resp: module <%d>\n", now(), id_val, cfg_req.fl.module);
            if (cfg_req.fl.packet_type == 0x11) {
              if ((cfg_req.fl.module < MIM_MODULE_NUM) && (cfg_req.fl.module >= 1)){
                if (radio_rx_get_settings(&radio[cfg_req.fl.module - 1]) == 0){
                  rx_cfg.fl.id = 20;
                  rx_cfg.fl.packet_type = 0x12;
                  rx_cfg.fl.packet_size = 326;
                  rx_cfg.fl.module = cfg_req.fl.module;
                  rx_cfg.fl.reserve = 0;
                  for (int k=0; k<4; k++) rx_cfg.fl.rx_set[k] = radio[cfg_req.fl.module - 1].rx_ctrl.IVarRxSet_rd[k];
                  mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_RX_CFG_RD, rx_cfg.tr_lvl.array, sizeof(rx_cfg.tr_lvl.array));
                }
              }
              else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            }
            else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            break;
          case MIM_INF_ID_RX_CFG_WR:
            memcpy((uint8_t*)&rx_cfg, mim.tr_lvl[num].data_map->unit[cmd_num].data.array, sizeof(type_MIM_INF_ID_RX_CFG));
            printf("%s: MIM cmd <%d> rx cfg wr: module <%d>\n", now(), id_val, cfg_req.fl.module);
            if (rx_cfg.fl.packet_type == 0x13) {
              if ((rx_cfg.fl.module < MIM_MODULE_NUM) && (rx_cfg.fl.module >= 1)){
                radio_rx_set_settings(&radio[rx_cfg.fl.module - 1], &rx_cfg.fl.rx_set[0]);
              }
              else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            }
            else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
            break;
          case MIM_INF_ID_EXP_DATA_REQ:
            memcpy((uint8_t*)uint8_array, mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.array, 6);
            if(*(uint16_t*)&uint8_array[0] == 0xABEF){
              nand_lin_read_block_set(ex.nand_ptr, *(uint32_t*)&uint8_array[2]);
              nand_lin_read_frame(ex.nand_ptr, ex.rx_nand_buffer);
              mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_EXP_DATA_RD, ex.rx_nand_buffer, NAND_FRAME_SIZE);
            }
            break;
          case MIM_INF_ID_MFR_SEGM_DATA_WR:
            printf("%s: MIM cmd %d segm_data: redirect to ID-%d \n", now(), id_val, mim.tr_lvl[num].data_map->unit[cmd_num].data.fields.id);
						// перенаправление данных на нужный ID как команду
            if (mim_tr_redirect_segm_data_cmd(&mim.tr_lvl[num])) {}
            else mim_error_collector(&mim, MIM_ERROR_CMD_PARAM, MIM_ERROR_TYPE_SET);
          break;
        }
        HAL_GPIO_WritePin(GPIOC, 1<<13, GPIO_PIN_RESET);
        //
      }
      id_val = mim_tr_get_processed_data_id(&mim.tr_lvl[num]);
			cmd_num = mim.tr_lvl[num].data_map->id_map[id_val];
      if (id_val >= 0){
        HAL_GPIO_WritePin(GPIOC, 1<<13, GPIO_PIN_SET);
        //
        switch (id_val){
            case MIM_INF_ID_RX_DATA_RD:
							printf("%s: MIM data <%d> rx data read\n", now(), id_val);
              rx_packet.fl.packet_type = 0x14;
							if(mim_read_rx_fifo(&mim, &rx_packet)){
                rx_packet.fl.packet_size = 4 + rx_packet.fl.packet.tot_size;
							}
							else{
								rx_packet.fl.packet_size = 4;
							}
              mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_RX_DATA_RD, (uint8_t*)&rx_packet.tr_lvl.array, rx_packet.fl.packet_size);
							break;
            case MIM_INF_ID_MFR_DATA_RD:
							printf("%s: MIM data <%d> mfr rx data read\n", now(), (int)id_val);
							mim_read_rx_fifo_and_form_mrf_packet(&mim, uint8_array);
              mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_MFR_DATA_RD, uint8_array, 1014);
							break;
            case MIM_INF_ID_EXP_DATA_RD:
							printf("%s: EXP data <%d>\n", now(), (int)id_val);
              nand_lin_read_frame(ex.nand_ptr, ex.rx_nand_buffer);
              mem_tr_inf_receive_id_data(mim.tr_lvl[0].data_map, MIM_INF_ID_EXP_DATA_RD, ex.rx_nand_buffer, NAND_FRAME_SIZE);
							break;
        }
      //
      HAL_GPIO_WritePin(GPIOC, 1<<13, GPIO_PIN_RESET);
      }
    }
		HAL_IWDG_Refresh(&hiwdg);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMLOW);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_SYSTICK_Callback(void)
{
	systick_counter++;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)  //TODO: необходимо сделать восстановление работы второй линии по прерыванию UART IDLE, так как на этом прерывании возникает коллизия при передаче
{
  if(huart == &huart7){ //RS_485 channel МА1
		mim_tr_it_tx_cb(&mim.tr_lvl[0]);
    // mim_tr_ena_communication(&mim.tr_lvl[2]);
	}
  if(huart == &huart4){ //RS_485 channel МА2
		// mim_tr_it_tx_cb(&mim.tr_lvl[2]);
    // mim_tr_ena_communication(&mim.tr_lvl[0]);
	}
  if(huart == &huart8){ //RS_485 channel МФР1
		mim_tr_it_tx_cb(&mim.tr_lvl[1]);
    // mim_tr_ena_communication(&mim.tr_lvl[3]);
	}
  if(huart == &huart5){ //RS_485 channel МФР2
		// mim_tr_it_tx_cb(&mim.tr_lvl[3]);
    // // mim_tr_ena_communication(&mim.tr_lvl[1]);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart == &huart1){ //system uart
		suart_rx_irq_handler(&suart);
	}
  if(huart == &huart7){ //RS_485 channel МА1
		mim_tr_it_rx_uart_cb(&mim.tr_lvl[0]);
    mim_tr_inh_communication(&mim.tr_lvl[2]);
	}
  if(huart == &huart4){ //RS_485 channel МА2
		// mim_tr_it_rx_uart_cb(&mim.tr_lvl[2]);
    // mim_tr_inh_communication(&mim.tr_lvl[0]);
	}
  if(huart == &huart8){ //RS_485 channel МФР1
		mim_tr_it_rx_uart_cb(&mim.tr_lvl[1]);
    mim_tr_inh_communication(&mim.tr_lvl[3]);
	}
  if(huart == &huart5){ //RS_485 channel МФР2
		// mim_tr_it_rx_uart_cb(&mim.tr_lvl[3]);
    // mim_tr_inh_communication(&mim.tr_lvl[1]);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if(huart == &huart7){ //RS_485 channel МА1
		printf("%s: uart 7 error %d\n", now(), (int)HAL_UART_GetError(huart));
    mim_tr_it_rx_error_cb(&mim.tr_lvl[0]);
    // mim_tr_ena_communication(&mim.tr_lvl[0]);
	}
  if(huart == &huart4){ //RS_485 channel МА2
		// printf("%s: uart 4 error %d\n", now(), (int)HAL_UART_GetError(huart));
    // mim_tr_it_rx_error_cb(&mim.tr_lvl[2]);
    // // mim_tr_ena_communication(&mim.tr_lvl[2]);
	}
  if(huart == &huart8){ //RS_485 channel МФР1
		printf("%s: uart 8 error %d\n", now(), (int)HAL_UART_GetError(huart));
    mim_tr_it_rx_error_cb(&mim.tr_lvl[1]);
    // mim_tr_ena_communication(&mim.tr_lvl[1]);
	}
  if(huart == &huart5){ //RS_485 channel МФР2
		// printf("%s: uart 5 error %d\n", now(), (int)HAL_UART_GetError(huart));
    // mim_tr_it_rx_error_cb(&mim.tr_lvl[3]);
    // // mim_tr_ena_communication(&mim.tr_lvl[3]);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim4) { //RS_485 channel МА1
		mim_tr_it_rx_timer_cb(&mim.tr_lvl[0]);
	}
	if (htim == &htim5) { //RS_485 channel МА2
		// mim_tr_it_rx_timer_cb(&mim.tr_lvl[2]);
	}
  if (htim == &htim3) { //RS_485 channel МФР1
		mim_tr_it_rx_timer_cb(&mim.tr_lvl[1]);
	}
  if (htim == &htim2) { //RS_485 channel МФР2
		// mim_tr_it_rx_timer_cb(&mim.tr_lvl[3]);
	}
  if (htim == &htim9) { // system_uart
		suart_timer_irq_handler(&suart);
	}
  if (htim == &htim7) { // task_planner
		tp_timer_handler(&tp);
	}
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  printf("GPIO pin %d\n", (int)GPIO_Pin);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
