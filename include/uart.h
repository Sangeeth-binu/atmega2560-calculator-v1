#ifndef UART_H
#define UART_H
#include <stdint.h>

void init_uart(void);
void uart_transmitt(char data);
void uart_print(char* str);
void uart_print_num(uint16_t num);

#endif
