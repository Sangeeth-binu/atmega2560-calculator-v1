#include <stdint.h>

#define F_CPU 16000000UL
#define CYCLES_PER_LOOP 10UL
#define DELAY_COUNT (F_CPU / 1000UL/ CYCLES_PER_LOOP)
/*
const uint8_t symbols[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,0x7f,0x67};
static uint8_t buffer[3] = {0,0,0};
static  uint8_t disp_len;
*/
volatile uint8_t*  PORTA = (volatile uint8_t*)0x22;//OUTPUT FOR KEYPAD
volatile uint8_t*  DDRA = (volatile uint8_t*)0x21;

volatile uint8_t* PINB = (volatile uint8_t*)0x23;//INPUT FOR KEYPAD
volatile uint8_t* DDRB = (volatile uint8_t*)0x24;
/*
volatile uint8_t* PORTK = (volatile uint8_t*)0x108;//SEVEN SEGMENT DATA
volatile uint8_t* DDRK = (volatile uint8_t*)0x107;

volatile uint8_t* PORTF = (volatile uint8_t*)0x31;//SEVEN SEGMENT SELECT
volatile uint8_t* DDRF = (volatile uint8_t*)0x30;
*/
volatile uint8_t* PORTC = (volatile uint8_t*)0x28;//ROW
volatile uint8_t* DDRC = (volatile uint8_t*)0x27;

volatile uint8_t* PORTD = (volatile uint8_t*)0x2B;//COL
volatile uint8_t* DDRD = (volatile uint8_t*)0x2A;
 
//static inline void display(uint16_t value);
static inline void delay_ms(volatile unsigned long ms);
static void init(void);
static void ScanKeypad(void);
static void SetNumber(uint8_t row, uint8_t col);

int main(void)
{
    init();

    while (1) {
        ScanKeypad();
    }
    return 0;
}


//---------------------------------------[FUNCTION-DOMAIN]---------------------------------------//

static void SetNumber(uint8_t r , uint8_t c)
{
        *PORTD = (1<<c);
        *PORTC = (1<<r);
}

static void ScanKeypad(void)
{
    uint8_t col,row;
    for(uint8_t i = 0;i<4;i++)
    {
        *PORTA = (1<<i);
        delay_ms(10);
        switch (*PINB) {
            case 0x00 : return;
            case 0x01 : col = 0; break;
            case 0x02 : col = 1; break;
            case 0x04 : col = 2; break;
            case 0x08 : col = 3; break;
            default   : return;
        }
        if(*PINB & 0x0f)
        {
            row = i;
            delay_ms(10);
        }
    }
    
    SetNumber(row,col);
}

static void init(void)
{
    *DDRA |= 0x0f;
    *DDRB |= 0x00;
    *DDRC |= 0x0f;
    *DDRD |= 0x0f;
}

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
//==================================================================================//
        /*for(i = 0;i<4;i++)
        {
            *PORTA = (1<<i);
            *PORTD = *PINB;
            uint8_t c = (*PINB & 0x0f);//we only pull downed the lower niubble ;)
            if(c)
            {
                *PORTC = (1<<i);
                delay_ms(1000);

            }
            *PORTC =0x00;
        }*/

