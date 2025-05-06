  /**
  ******************************************************************************
  * @file           : debug.c
  * @version        : v1.0
  * @brief          : функции для отладки, отключаемы define DEBUG
  * @author			: Стюф Алексей/Alexe Styuf <a-styuf@yandex.ru>
  ******************************************************************************
  */

#include "debug.h"

struct __FILE {
  int handle;
};
// FILE __stdout;

int fputc(int ch, FILE *f) {
  #ifdef DEBUG
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
  #else 
		return ch;
  #endif
}

int ferror(FILE *f) {
  return 0;
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
