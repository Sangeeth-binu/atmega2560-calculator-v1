#include <stdint.h>
#include "uart.h"

#define F_CPU 16000000UL
#define CYCLES_PER_LOOP 10UL
#define DELAY_COUNT (F_CPU / 1000UL/ CYCLES_PER_LOOP)

#define E 11
#define I 10
#define N 12
#define F 13
#define BLANK 14

//------------------------------------------------[PORTS]--------------------------------------------------//

volatile uint8_t*  PORTA = (volatile uint8_t*)0x22;//OUTPUT FOR KEYPAD
volatile uint8_t*  DDRA = (volatile uint8_t*)0x21;

volatile uint8_t* PINB = (volatile uint8_t*)0x23;//INPUT FOR KEYPAD
volatile uint8_t* DDRB = (volatile uint8_t*)0x24;


volatile uint8_t* PORTK = (volatile uint8_t*)0x108;//SEVEN SEGMENT DATA
volatile uint8_t* DDRK = (volatile uint8_t*)0x107;

volatile uint8_t* PORTF = (volatile uint8_t*)0x31;//SEVEN SEGMENT SELECT
volatile uint8_t* DDRF = (volatile uint8_t*)0x30;

volatile uint8_t* PORTC = (volatile uint8_t*)0x28;//ROW
volatile uint8_t* DDRC = (volatile uint8_t*)0x27;

volatile uint8_t* PORTD = (volatile uint8_t*)0x2B;//COL
volatile uint8_t* DDRD = (volatile uint8_t*)0x2A;
 
//-----------------------------------------------------------------------------------------------------------//
//                                        [FUNCTIONS]                                                        //

static void action(void);
//static inline void display(uint16_t value);
static inline void delay_ms(volatile unsigned long ms);
//static void init(void);
static void ScanKeypad(void);
static void SetNumber(uint8_t row, uint8_t col);
static void fill_number(uint8_t data);
static inline void debounce(void);

static void add(void);
static void sub(void);
static void mul(void);
static void div(void);

static void dot(void);
static void equ(void);

static void debug(void);
//------------------------------------------------------------------------------------------------------------//
//                                         [ELEMENTS]                                                         //

const uint8_t symbols[15] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,0x7f,0x67,0x79,0x06,0x3f,0x71, 0x00};
static uint8_t buffer[3] = {BLANK,BLANK,BLANK};
//static  uint8_t disp_len;
static uint8_t col= 0xff, row=0xff;
static uint8_t last_col = 0xff, last_row = 0xff;
void (*op)(void);
static uint8_t operands[2] = {0,0};
static uint8_t flag = 0, proceed = 0;
static uint8_t key_locked = 0;


typedef enum {
    KEY_NUMBER,
    KEY_FUNCN
} key_type;


typedef struct {
    key_type key;                          //TAG -- for tagged union since union doesnt know whaat it is storing
    union {
        uint8_t num;
        void (*func)(void);
    } data;
} cell;

cell KeyPad[4][4] = {
    {
        { .key = KEY_NUMBER, .data.num = 1},{ .key = KEY_NUMBER, .data.num = 2},{ .key = KEY_NUMBER, .data.num = 3},{ .key = KEY_FUNCN, .data.func = add}
    },
    {
        { .key = KEY_NUMBER, .data.num = 4},{ .key = KEY_NUMBER, .data.num = 5},{ .key = KEY_NUMBER, .data.num = 6},{ .key = KEY_FUNCN, .data.func = sub}
    },
    {
        { .key = KEY_NUMBER, .data.num = 7},{ .key = KEY_NUMBER, .data.num = 8},{ .key = KEY_NUMBER, .data.num = 9},{ .key = KEY_FUNCN, .data.func = mul}
    },
    {
        { .key = KEY_FUNCN, .data.func = dot},{ .key = KEY_NUMBER, .data.num = 0},{ .key = KEY_FUNCN, .data.func = equ },{ .key = KEY_FUNCN, .data.func = div}
    },
};

//------------------------------------------------------------------------------------------------------//
//                                             [MAIN]                                                   //

int main(void)
{
//  init();
    *DDRA = 0xff;
    *DDRB = 0x00;
    *DDRC = 0xff;
    *DDRD = 0xff;
    flag= 0;
    init_uart();
    uart_print("UART OK\r\n");

        uart_print_num(buffer[2]);
        uart_print("\n \r");
        uart_print_num(buffer[1]);
        uart_print("\n \r");
        uart_print_num(buffer[0]);
        uart_print("\n \r");


    while (1) {
        action();
    }
    return 0;
}


//---------------------------------------------------------------------------------------------------//
//                                      [FUNCTION DOMAIN]                                            //

static void debug(void)
{
        uart_print("Key pressed \n \r");
        uart_print_num(row);
        uart_print("\n \r");
        uart_print_num(col);
        uart_print("\n \r");
        last_row = row;
        last_col = col;
        uart_print_num(buffer[2]);
        uart_print("\n \r");
        uart_print_num(buffer[1]);
        uart_print("\n \r");
        uart_print_num(buffer[0]);
        uart_print("\n \r");
        uart_print("\n \t operands \t 1] ");
        uart_print_num(operands[0]);
        uart_print(" 2] ");
        uart_print_num(operands[1]);
        uart_print("\n \r");
}

static inline void debounce(void)
{
    delay_ms(200);
}

static void fill_number(uint8_t data)
{
    if(col == 0xff || row == 0xff || !((row < 3 && col < 3) || (row == 3 && col == 1)))
        return;

    if((row < 3 && col < 3) || (row == 3 && col == 1)){
        if( buffer[2] == BLANK )
        {
            buffer[2] = buffer[1];
            delay_ms(30);
            buffer[1] = buffer[0];
            delay_ms(30);
            buffer[0] = data;

            if(flag == 0)
                operands[0] = operands[0]*10 + data;
            else if (flag == 1 && proceed == 0)
                operands[1] = operands[1]*10 + data;

        } else {
            buffer[2] = E;
            buffer[1] = E;
            buffer[0] = E;
        }
    }
}
static void SetNumber(uint8_t r , uint8_t c)
{
    switch (r) {
        case 1: r = 0; break;
        case 2: r = 1; break;
        case 4: r = 2; break;
        case 8: r = 3; break;
    }

    switch (c) {
        case 1: c = 0; break;
        case 2: c = 1; break;
        case 4: c = 2; break;
        case 8: c = 3; break;
    }

    col = c;
    row = r;

    *PORTD = (1<<c);
    *PORTC = (1<<r);
}

static void ScanKeypad(void)
{
    for(volatile uint8_t i = 0; i<4;i++)
    {
        *PORTA = (1<<i);
        if ((*PINB & 0x0f) != 0 && !key_locked)
        {
            SetNumber((1<<i), (*PINB & 0x0f));
            key_locked = 1;
        }

        if ((*PINB & 0x0f) == 0)
        {
            key_locked = 0;
        }

    }
    return;
}   



static void action(void)
{
    ScanKeypad();
   
    if (row == 0xff || col == 0xff)
        return;

    debounce();

    cell *k = &KeyPad[row][col];

    switch (k->key) {
        case KEY_NUMBER : fill_number(k->data.num);
                          break;
        case KEY_FUNCN  : (k->data.func)();
                          break;
    }

    debug();
    key_locked = 0;
    col= 0xff; row=0xff;
}


static void add(void)
{
    uart_print("detected the operation add\n \r");
    op = add;
    flag = 1;
    buffer[0] = BLANK; buffer[1] = BLANK; buffer[2] = BLANK;
    col = 0xff; row = 0xff;
    return;
}
static void sub(void)
{    
    uart_print("detected the operation sub \n \r");
    op = sub;
    flag = 1;
    buffer[0] = BLANK; buffer[1] = BLANK; buffer[2] = BLANK;
    col = 0xff; row = 0xff;
    return;
}
static void mul(void)
{
    uart_print("detected the operation mul \n \r");
    buffer[0] = BLANK; buffer[1] = BLANK; buffer[2] = BLANK;
    op = mul;
    flag = 1;
    col = 0xff; row = 0xff;
    return;
}
static void div(void)
{
    uart_print("detected the operation div \n \r");
    op = div;
    flag = 1;
    buffer[0] = BLANK; buffer[1] = BLANK; buffer[2] = BLANK;
    col = 0xff; row = 0xff;
    return;
}
static void equ(void)
{

    int result;
    if (op == add)
    {    result = operands[0] + operands[1];}
    else if (op == sub)
    {   result = operands[0] - operands[1];}
    else if (op == mul)
    {   result = operands[0] * operands[1];}
    else if (op == div)
    {   result = operands[1] ? operands[0] / operands[1] : 9999;}
    else {
        result = 9999;
    }
    if(result > 999 || result < (-999))
    {
        buffer[2] = E; buffer[1] = E; buffer[0] = E;
    } else {
        buffer[2] = result/100;
        buffer[1] = (result/10)%10;
        buffer[0] = result%10;
    }
    flag = 0;

    col = 0xff;
    row = 0xff;
    operands[0] = 0; operands[1] = 0;

    return;
}
static void dot(void){
       buffer[0] = BLANK; buffer[1] = BLANK; buffer[2] = BLANK;    
       operands[0] = 0; operands[1] = 0;
       flag = 0;
};

/*
static inline void display(uint16_t value)
{
    uint8_t idx = 0;

    *PORTF |= 0x07;
    *PORTK |= symbols[buffer[idx]];
    *PORTF &= ~(1<<idx);

    (idx >= disp_len) ? idx = 0 : idx++;
}
*/
static inline void delay_ms(volatile unsigned long ms)
{
    for(unsigned long i = 0; i<ms; i++)
    {
        for(unsigned long j = 0; j < DELAY_COUNT; j++)
        {
            __asm__ __volatile__("nop");
        }
    }
}
//===============================================================================================//
 
