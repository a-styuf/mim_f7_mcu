  /**
  ******************************************************************************
  * @file           : debug.c
  * @version        : v1.0
  * @brief          : функции для отладки, отключаемы define DEBUG
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "debug.h"

// int _write(char *ptr, int len ) 
// {
//   return _writeUart(ptr, len);
// }

// int _writeUart( char *ptr, int len )
// {
//   HAL_UART_Transmit(&huart1, (uint8_t*) ptr, len, 0xFFFF);
//   return 0;
// }


/**
  * @brief  вывод времени в printf
  */
void printf_time(void)
{
	RTC_TimeTypeDef time;
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	printf("%02d:%02d:%02d.%03d ", time.Hours, time.Minutes, time.Seconds, time.SecondFraction - time.SubSeconds); 
}

/**
  * @brief  вывод на экран массива hex-значений
  * @param  buff: указатель на блок памяти
  * @param  len: длина данных
  * @param  end_char: символ окончания вывода
  */
void printf_buff(uint8_t *buff, uint8_t len, char end_char)
{
  for (uint8_t i=0; i<len; i++){
    printf("%02X ", buff[i]);
  }
  printf("%c", end_char);
}
