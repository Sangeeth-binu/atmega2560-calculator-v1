//______________________________________________CALCULATOR____________________________________________________//



#include <stdint.h>
#include "uart.h"



//-----------------------------------------------[MACROS]----------------------------------------------------//



#define F_CPU 16000000UL
#define CYCLES_PER_LOOP 10UL
#define DELAY_COUNT (F_CPU / 1000UL/ CYCLES_PER_LOOP)

#define E 10
#define I 11
#define N 12
#define F 13
#define BLANK 14
#define MINUS 15

#define DISP_DIGITS 2


//------------------------------------------------[PORTS]--------------------------------------------------//



volatile uint8_t*  PORTA = (volatile uint8_t*)0x22;//OUTPUT FOR KEYPAD
volatile uint8_t*  DDRA = (volatile uint8_t*)0x21;

volatile uint8_t* PINB = (volatile uint8_t*)0x23;//INPUT FOR KEYPAD
volatile uint8_t* DDRB = (volatile uint8_t*)0x24;

volatile uint8_t* PORTK = (volatile uint8_t*)0x108;//SEVEN SEGMENT SELECT
volatile uint8_t* DDRK = (volatile uint8_t*)0x107;

volatile uint8_t* PORTF = (volatile uint8_t*)0x31;//SEVEN SEGMENT DATA
volatile uint8_t* DDRF = (volatile uint8_t*)0x30;

volatile uint8_t* PINL = (volatile uint8_t*)0x109;//SWITCH
volatile uint8_t* DDRL = (volatile uint8_t*)0x10a;



//----------------------------------------------[FUNCTIONS]-------------------------------------------------//



static void process_input(void);
static inline void display(void);
static inline void delay_ms(volatile unsigned long ms);
//static void init(void);
static void scan_keypad(void);
static void decode_key_position(uint8_t row, uint8_t col);
static void fill_number(uint8_t data);
static inline void debounce(void);

static void add(void);
static void sub(void);
static void mul(void);
static void div(void);

static uint16_t power(uint16_t x, uint16_t y);

static void handle_clear_key(void);
static void handle_equals_key(void);

static void debug(void);

static void capture_decimal_point(void);
static int format_digit( int result, uint8_t result_frac_digit);


//---------------------------------------------[ELEMENTS]--------------------------------------------------//

//-------[[DISPLAY RELATED VARIABLES]]

static uint8_t symbols[16] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,0x7f,0x67,0x79,0x06,0x3f,0x71, 0x00, 0x40};
static uint8_t disp_digits[3] = {BLANK,BLANK,BLANK};
static uint8_t dp_index = 0xff;

//-------[[INPUT/KEYPAD STATE VARIABLES]]

static uint8_t key_col= 0xff, key_row=0xff;
static uint8_t prev_key_col = 0xff, prev_key_row = 0xff;
static uint8_t key_latched = 0;

//-------[[OPERAND & MATH STATE]]

void (*pending_op)(void);
static uint16_t operand[2] = {0,0};
static uint8_t active_operand = 0;

//-------[[FIXED-POINT TRACKING]]

static uint8_t frac_digit_count[2] = {0,0};
static uint16_t fp_scale = 0;

//-------[[MODE / CONTROL STATE]]

static uint8_t  dp_locked = 0;

typedef enum {
    KEY_NUMBER,
    KEY_FUNCN
} key_type;


typedef struct {
    key_type key;                          //TAG -- for tagged union since union doesnt know what it is storing
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
        { .key = KEY_FUNCN, .data.func = handle_clear_key },{ .key = KEY_NUMBER, .data.num = 0},{ .key = KEY_FUNCN, .data.func = handle_equals_key },{ .key = KEY_FUNCN, .data.func = div}
    },
};



//----------------------------------------------[MAIN]---------------------------------------------//



int main(void)
{
    //init();
    *DDRA = 0xff;
    *DDRB = 0x00;

    *DDRF = 0xff;
    *DDRK = 0xff;

    *DDRL = 0X00;

    active_operand = 0;

    init_uart();
    uart_print("UART OK\r\n");

    uart_print_num(disp_digits[2]);
    uart_print("\n \r");
    uart_print_num(disp_digits[1]);
    uart_print("\n \r");
    uart_print_num(disp_digits[0]);
    uart_print("\n \r");

    while (1) {
        process_input();
        display();
    }
    return 0;
}



//---------------------------------------[FUNCTION DOMAIN]-------------------------------------------//



//static void init(void)
//{
//}

static void capture_decimal_point(void)
{
    dp_locked = 1;
    return;
}

static void debug(void)
{
    uart_print("------------------------------------\n\rKey pressed \n \r");
    uart_print_num(key_row);
    uart_print("\n \r");
    uart_print_num(key_col);
    uart_print("\n \r");
    prev_key_row = key_row;
    prev_key_col = key_col;
    uart_print_num(disp_digits[2]);
    uart_print("\n \r");
    uart_print_num(disp_digits[1]);
    uart_print("\n \r");
    uart_print_num(disp_digits[0]);
    uart_print("\n \r");
    uart_print("\n \t operands \t 1] ");
    uart_print_num(operand[0]);
    uart_print(" 2] ");
    uart_print_num(operand[1]);
    uart_print("\n \r keys pressed after decimal point = ");
    uart_print(" op1] ");
    uart_print_num(frac_digit_count[0]);
    uart_print(" | op2] ");
    uart_print_num(frac_digit_count[1]);
    uart_print("\n\n \r dp_index == ");
    uart_print_num(dp_index);
    uart_print("\t dp_locked or not ==");
    uart_print_num(dp_locked);
    uart_print(" \n \r--------------------------------------------\n\r");

}

static inline void debounce(void)
{
    delay_ms(200);
}

static void fill_number(uint8_t data)
{
    if(key_col == 0xff || key_row == 0xff || !((key_row < 3 && key_col < 3) || (key_row == 3 && key_col == 1)))
        return;

    if((key_row < 3 && key_col < 3) || (key_row == 3 && key_col == 1)){
        if( disp_digits[2] == BLANK )
        {
            disp_digits[2] = disp_digits[1];
            delay_ms(30);
            disp_digits[1] = disp_digits[0];
            delay_ms(30);
            disp_digits[0] = data;
            operand[active_operand] = operand[active_operand]*10 + data;
        } else {
            disp_digits[2] = E;
            disp_digits[1] = E;
            disp_digits[0] = E;
        }
    }
}

static void decode_key_position(uint8_t r , uint8_t c)
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

    key_col = c;
    key_row = r;
}

static void scan_keypad(void)
{
    for(volatile uint8_t i = 0; i<4;i++)
    {
        *PORTA = (1<<i);
        if ((*PINB & 0x0f) != 0 && !key_latched)
        {
            decode_key_position((1<<i), (*PINB & 0x0f));
            key_latched = 1;
        }
 
    }

    if( (*PINL & 0x01) != 0 )
        capture_decimal_point();
    
    key_latched = 0;
    return;
}   



static void process_input(void)
{
    scan_keypad();

    if (key_row == 0xff || key_col == 0xff)
        return;

    debounce();

    cell *k = &KeyPad[key_row][key_col];

    switch (k->key) {
        case KEY_NUMBER : fill_number(k->data.num);
                          if( dp_locked )
                          {
                              ++frac_digit_count[active_operand];
                              dp_index = frac_digit_count[active_operand];
                          }
                          break;
        case KEY_FUNCN  : (k->data.func)();
                          dp_locked = 0;
                          break;
    }

//    if( dp_locked )
//        dp_index = frac_digit_count[active_operand] - 1;

    debug();
    key_latched = 0;
    key_col= 0xff; key_row=0xff;
}


static void add(void)
{
    uart_print("detected the operation add\n \r");
    pending_op = add;
    active_operand = 1;
    disp_digits[0] = BLANK; disp_digits[1] = BLANK; disp_digits[2] = BLANK;
    key_col = 0xff; key_row = 0xff;
    dp_locked = 0; dp_index = 0xff;
    return;
}
static void sub(void)
{    
    uart_print("detected the operation sub \n \r");
    pending_op = sub;
    active_operand = 1;
    disp_digits[0] = BLANK; disp_digits[1] = BLANK; disp_digits[2] = BLANK;
    key_col = 0xff; key_row = 0xff;
    dp_locked = 0; dp_index = 0xff;
    return;
}
static void mul(void)
{
    uart_print("detected the operation mul \n \r");
    disp_digits[0] = BLANK; disp_digits[1] = BLANK; disp_digits[2] = BLANK;
    pending_op = mul;
    active_operand = 1;
    key_col = 0xff; key_row = 0xff;
    dp_locked = 0; dp_index = 0xff;
    return;
}
static void div(void)
{
    uart_print("detected the operation div \n \r");
    pending_op = div;
    active_operand = 1;
    disp_digits[0] = BLANK; disp_digits[1] = BLANK; disp_digits[2] = BLANK;
    key_col = 0xff; key_row = 0xff;
    dp_locked = 0; dp_index = 0xff;
    return;
}

static void handle_equals_key(void)
{

    int32_t result;

    int32_t tmp_a = 0;
    int32_t tmp_b = 0;

    uint8_t result_frac_digits = 0;
    uint8_t common_frac = 0;

    /* ------------------ determine common fractional scale ------------------ */
    common_frac = (frac_digit_count[0] > frac_digit_count[1])
                    ? frac_digit_count[0]
                    : frac_digit_count[1];

    /* ---------------fixed point arithmetric ---------------- */

        //real_value = integer_value / (10 ^ frac_digit_count)
        //Addition and subtraction only work when denominators are equal:
        //1.Choose a common denominator
        //2.Convert each operand to that denominator
        //3.Then operate
        //that is we need to equalize the denominastor
        //
        //multiplication is simple
        //
        //in division the integer division truncates the result automatilay
        //therefore we need to scale the numerator first with our own precision
        //thne do the division to preserve the fractional part which may 
        //sometime get lost due tto the normal integer division logic

    /* ------------------ perform operation ------------------ */
    if (pending_op == add)
    {
        tmp_a = (int32_t)operand[0] * power(10, common_frac - frac_digit_count[0]);
        tmp_b = (int32_t)operand[1] * power(10, common_frac - frac_digit_count[1]);

        result = tmp_a + tmp_b;
        result_frac_digits = common_frac;
    }
    else if (pending_op == sub)
    {
        tmp_a = (int32_t)operand[0] * power(10, common_frac - frac_digit_count[0]);
        tmp_b = (int32_t)operand[1] * power(10, common_frac - frac_digit_count[1]);

        result = tmp_a - tmp_b;
        result_frac_digits = common_frac;
    }
    else if (pending_op == mul)
    {
        tmp_a = (int32_t)operand[0];
        tmp_b = (int32_t)operand[1];

        result = tmp_a * tmp_b;
        result_frac_digits = frac_digit_count[0] + frac_digit_count[1];
    }
    else if (pending_op == div)
    {
        const uint8_t DIV_PRECISION = 2;

        if (operand[1] != 0)
        {
            tmp_a = (int32_t)operand[0] * power(10, DIV_PRECISION + frac_digit_count[1]);
            tmp_b = (int32_t)operand[1] * power(10, frac_digit_count[0]);

            result = tmp_a / tmp_b;
            result_frac_digits = DIV_PRECISION;
        }
        else
        {
            result = 9999;
            result_frac_digits = 0;
        }
    }
    else
    {
        result = 9999;
        result_frac_digits = 0;
    }

    /* ------------------ format result for display ------------------ */
    result = format_digit(result, result_frac_digits);

    /* ------------------------ updations ---------------------------*/
    if( dp_index == 0 )
        dp_locked = 0;


    if(result > 999 || result < (-99))
    {
        disp_digits[2] = E; disp_digits[1] = E; disp_digits[0] = E;
    } else {
        if(result < 0)
        {
            uint16_t temp = -result;
            disp_digits[2] = (temp >= 10) ? MINUS : BLANK;
            disp_digits[1] = (temp < 10) ? MINUS : temp/10;
            disp_digits[0] = temp%10;
        } else {
            disp_digits[2] = (result >= 100) ? result/100 : BLANK;
            disp_digits[1] = (result < 10 && result >= 0) ? BLANK : (result/10)%10;
            disp_digits[0] = result%10;
        }
    }
    

    active_operand = 0;

    key_col = 0xff;
    key_row = 0xff;
    operand[0] = 0; operand[1] = 0;
    dp_locked = 0;
    return;
}

static void handle_clear_key(void){
       disp_digits[0] = BLANK; disp_digits[1] = BLANK; disp_digits[2] = BLANK;    
       operand[0] = 0; operand[1] = 0;
       frac_digit_count[1] = 0; frac_digit_count[0] = 0;
       fp_scale = 0;
       dp_locked = 0;
       active_operand = 0;
       dp_index = 0xff;
};

static int format_digit( int result , uint8_t result_frac)
{
    int value = result;

    if( result < 0 )
    {
        value = -result;
        while (value > 99 ) {
            value /= 10;
            result_frac--;
        }
        value = -value;
    } else {
        while (value > 999) {
            value /= 10;
            result_frac--;
        }
    }

    dp_index = result_frac;
    return value;
}
static inline void display(void)
{
    static uint8_t scan_index = 0;

    *PORTK = 0x07;                                   //DISPLAY DISABLED ; OUTPUT ONE FOR LOWER THREE BITS
    if( scan_index == dp_index && dp_index != 0xff){
        *PORTF = symbols[disp_digits[scan_index]]|0x80; 
        *PORTK &= ~(1<<scan_index);                              
    } else {
        *PORTF = symbols[disp_digits[scan_index]];               //PASSING DATA OUT
        *PORTK &= ~(1<<scan_index);                             //ENABLING ONE DIGIT ; ONE BIT ZERO AT A TIME AMONG THE LOWER THREE BITS
    }
   // delay_ms(1);
    (scan_index >= DISP_DIGITS) ? scan_index = 0 : scan_index++;
}

static uint16_t power(uint16_t x, uint16_t y)
{
    uint16_t result = 1;
    while (y--) result *= x;
    return result;
}

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
 
