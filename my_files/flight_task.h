#ifndef _FLIGHT_TASK_H_
#define _FLIGHT_TASK_H_

//*** Includes ***//
#include "byteswap.h"
#include "crc16.h"
#include "clock.h"
#include "frames.h"
#include "task_planner.h"
#include "debug.h"

//*** Defines ***//
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

// определитель кадров (метка для визуального или автоматического поиска кадров)
#define FT_MARK                    0xFAFB
// параметры полетных заданий
#define FT_STEP_LEN_BYTE    (64)  //необходимо проверять длину шага
#define FT_LEN_STEP         (32)
#define FT_LEN_BYTE         (FT_LEN_STEP*FT_STEP_LEN_BYTE)

enum FT_MODE
{
  FT_MODE_OFF, FT_MODE_START, FT_MODE_WORK, FT_MODE_PAUSE, 
  FT_MODE_NUM
};

#define FT_FUNC_GROUP_NUM_MAX     (8)
#define FT_FUNCT_NUM_MAX          (16)

// настройки FIFO для полетного задания
#define FT_FIFO_DEPTH          (32)  //! глубина FIFO в кадрах
#define FT_FIFO_REC_LEN        (128)  //! длина единичной записи

#define FT_DATA_TO_SAVE_MAX_LEN (128)

// настройки шага
#define FT_STATUS_SAVE_DATA_ENA         (1<<0)  //! сохранение данных в fifo
#define FT_STATUS_SAVE_DATA_TYPE        (1<<1)  //! тип данных в fifo: 0 - функция передает сырые данные, которые пакуются в кадр, 1 - функция передает уже кадр, который сохраняется без изменения 
#define FT_STATUS_BRANCH_RPT_BREAK      (1<<2)  //! TODO: ветвление по статусу выполнения функции: выход из повтора
#define FT_STATUS_BRANCH_FT_BREAK       (1<<3)  //! TODO: ветвление по статусу выполнения функции: выход из полетного задания

// статусы, возвращаемые функцией полетного задания
#define FT_FUN_RET_BRANCH_RPT_BREAK     (1)     //! прерываем выполнение повтора шага полетного задания
#define FT_FUN_RET_NOPE                 (0)     //! не обрабатываем
#define FT_FUN_RET_BRANCH_FT_BREAK      (-1)    //! прерываем выполнение полетного задания

//*** General parts of frames ***//
#pragma pack(push, 2)


typedef union{
  uint8_t array[8];
  struct {
    uint8_t self_id;
    uint8_t mode;
    uint8_t function_info;
    uint8_t step_num;
    uint16_t rpt_value;
    uint16_t frame_num;
  }fields;
}typeFTReport;  // 8

/**
  * @brief  шаг полетного задания длина 64 байта фиксирована
  */
typedef union{
  uint8_t array[FT_STEP_LEN_BYTE];
  struct {
    uint16_t label;         // +0
    uint8_t type;           // +2
    uint8_t cmd;            // +3
    uint16_t rpt_cnt;       // +4
    uint8_t go_to;          // +6
    uint8_t go_cnt;         // +7
    uint32_t delay_ms;      // +8
    uint16_t settings;      // +10
    uint16_t reserve[8];    // +12
    uint8_t data[32];       // +30
    uint16_t crc16;         // +62
  }fields;
}typeFTStep;  // 64

/**
  * @brief  массив полетного задания
  */
typedef union
{
  typeFTStep  step[FT_LEN_STEP];
  uint8_t     array[FT_LEN_BYTE];
}typeFTArray;

/**
  * @brief  функции работы полетного задания
  */
typedef struct
{
  int32_t (*func) (void* ctrl_struct, uint8_t* ctrl_data, uint8_t* data_to_save, uint8_t* data_len);
  void* ctrl_struct;
}typeFTFunction;

/**
  * @brief  контейнер функций для работы полетного задания
  */
typedef struct
{
  typeFTFunction func[FT_FUNCT_NUM_MAX];
}typeFTFunctionGroup;

typedef struct{
  typeFTStep step_cast; //! слепок текущего шага
  // для циклирования используются переходы и счетчики переходов
  uint8_t go_to_cnt[FT_LEN_STEP]; //! глобальный счетчик переходов между шагами
  uint8_t go_to_max[FT_LEN_STEP]; //! значение переходов шагов всего полетного задания
  // управление шагом
  uint32_t last_step_time, step_delay;
  uint32_t pause_time, last_pause_time;
  uint8_t step_num;
  uint16_t rpt_value;
  // параметры вызова функций
  uint8_t data_to_save[FT_DATA_TO_SAVE_MAX_LEN], data_len;
  //
  typeSingleFrameStruct ft_frame;
  uint16_t frame_num;
  //
  int32_t func_ret_val;
} typeFTCtrl;

typedef struct{
  uint8_t array[FT_FIFO_DEPTH][FT_FIFO_REC_LEN];
  uint8_t rec_num, rec_max;
} typeFTFifo;

/**
  * @brief  
  */
typedef struct
{
  // общие настройки
  uint8_t self_num, sat_id, dev_id;
  typeFTArray task;
  //
  uint8_t mode;
  // вызываемые функции
  typeFTFunctionGroup f_group[FT_FUNC_GROUP_NUM_MAX];
  // управление работой циклограммы
  typeFTCtrl ctrl;
  //
  typeFTReport report;
  // fifo для хранения данных 
  typeFTFifo fifo;
  // текущее управление
  uint32_t last_call_time_ms;
}typeFT;

#pragma pack(pop)

//*** Function prototypes ***//

int8_t ft_init(typeFT *ft_ptr, uint8_t self_num, uint8_t sat_id, uint8_t dev_id);
int8_t ft_process_tp(void* ctrl_struct, uint64_t time_us, typeProcessInterfaceStruct* interface);
int8_t ft_process(typeFT *ft_ptr, uint32_t time_ms);
void ft_mode_work_process(typeFT *ft_ptr, uint32_t time_ms);
int8_t ft_create_step_cast(typeFT *ft_ptr);
int32_t ft_func_run(typeFT* ft_ptr);
int32_t ft_function_retval_check(typeFT* ft_ptr);
int8_t ft_check_step_cast_crc(typeFT *ft_ptr);
void __ft_step_cast_reset(typeFT *ft_ptr);
int8_t ft_check_step_cast_crc(typeFT *ft_ptr);
int8_t ft_function_registration(typeFT *ft_ptr, uint8_t group_num, uint8_t func_num, int32_t (*func) (void*, uint8_t*, uint8_t*, uint8_t*), void* ctrl_struct);
int8_t ft_set_mode(typeFT *ft_ptr, uint8_t mode);
int8_t ft_load_task(typeFT *ft_ptr, uint8_t *task_array);
void ft_refresh_go_to_parameters(typeFT *ft_ptr);
typeFTArray ft_unload_task(typeFT *ft_ptr);
int8_t ft_check_task(typeFT *ft_ptr);
int8_t ft_write_fifo(typeFT *ft_ptr, uint8_t* data);
int8_t ft_read_fifo(typeFT* ft_ptr, uint8_t* data);
typeFTReport ft_create_report(typeFT* ft_ptr);

//
void ft_create_ft_step(typeFTStep* step, uint8_t type, uint8_t cmd, uint16_t rpt_cnt, uint32_t delay_ms, uint16_t settings, uint8_t go_to, uint8_t go_cnt, uint8_t*data);

#endif
