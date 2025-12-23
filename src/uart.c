#include "uart.h"
#include <stdint.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU/(16UL * BAUD))-1)

//UCSR0C BITS
#define UCSZ00  1
#define UCSZ01  2

//UCSR0B BITS
#define  TXEN0  3
#define  RXEN0  4

//UCSR0A BITS
#define  UDRE0  5
#define  TXC0   6
#define  RXC0   7

typedef struct //struct defines the blueprint of the memory layout
{
    volatile uint8_t UCSRnA;      // offset +0
    volatile uint8_t UCSRnB;      // offset +1
    volatile uint8_t UCSRnC;      // offset +2
    uint8_t          _pad;        // offset +3
    volatile uint8_t UBRRnL;      // offset +4
    volatile uint8_t UBRRnH;      // offset +5
    volatile uint8_t UDRn;        // offset +6
}USART;


#define USART0 ((USART *)0xc0)

void init_uart(void)
{
    //1.setting baud rate
    USART0->UBRRnH = ( UBRR_VALUE >> 8 );
    USART0->UBRRnL = ( UBRR_VALUE & 0xff );

    //2.powering on Tx and Rx
    USART0->UCSRnB = (1 << RXEN0)|(1 << TXEN0);

    //3.configuring data points
    USART0->UCSRnC = (1<<UCSZ00)|(1<<UCSZ01);
}

void uart_transmitt(char data)
{
    while(!((USART0->UCSRnA)&(1<<UDRE0)));

    USART0->UDRn = data;
}

void uart_print(char* str)
{
    while(*str)
    {
        uart_transmitt(*str);
        str++;
    }
}

void uart_print_num(uint16_t num)
{
    if(num >= 10)
    {
        uart_print_num(num/10);
    }

    uart_transmitt((num%10) + '0');

}



