/* Initialize and display "Hello!" on the LCD sing 4-bit data mode. All interface uses Port B */
/* This program strictly follows HD44780 datasheet for timing. You may want to adjust the amount of delay for your LCD controller. */

#include "TM4C123GH6PM.h"

#define LIGHT GPIOE
#define SENSOR GPIOC
#define PEDX GPIOD
#define SSEG GPIOB

#define LCD_PORT GPIOA
#define RS 4 		/* A2 */
#define RW 0    /* GND*/
#define EN 8    /* A3*/

void delayUs(int n);
void LCD_nibble_write(unsigned char data, unsigned char control);
void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_init(void);
void delayMs(int n);  

// Linked data structure
struct State {
  unsigned long Out;
  unsigned long Time;  
  unsigned long Next[16];};

typedef const struct State STyp;
#define goN   0
#define waitN 1
#define goE   2
#define waitE 3
STyp FSM[4]={
 {0x21, 3000,{goN,waitN,goN,waitN,waitN,waitN,waitN,waitN,goN,waitN,goN,waitN,waitN,waitN,waitN,waitN}},
 {0x22, 3000,{goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE,goE}},
 {0x0C, 3000,{goE,goE,waitE,waitE,goE,goE,waitE,waitE,waitE,waitE,waitE,waitE,waitE,waitE,waitE,waitE}},
 {0x14, 3000,{goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN,goN}}};
 // index to the current state
 
int Input;
int inX;
int S;
int num;
int main(void){

SYSCTL->RCGCGPIO |= 0x1F;

LIGHT->DIR = 0x3F;
LIGHT->DEN = 0x3F;

SSEG->DIR = 0XFF;
SSEG->DEN = 0XFF;
 
SENSOR->DIR = 0x00;
SENSOR->PUR = 0xF0;
SENSOR->DEN = 0xF0;

PEDX->DIR = 0x00;
PEDX->DEN = 0xFF;

    /* configure PORTD6 for falling edge trigger interrupt */
    GPIOD->DIR &= ~0xFF;        /* make PORTD6 input pin */
    GPIOD->DEN |= 0xFF;         /* make PORTD6 digital pin */
    GPIOD->IS  &= ~0xFF;        /* make bit 4, 0 edge sensitive */
    GPIOD->IBE &= ~0xFF;        /* trigger is controlled by IEV */
    GPIOD->IEV &= ~0xFF;        /* falling edge trigger */
    GPIOD->ICR |= 0xFF;         /* clear any prior interrupt */
    GPIOD->IM  |= 0xFF;         /* unmask interrupt */
    GPIOD->PUR = 0xFF;
    NVIC->IP[3] = 6 << 5;       /* set interrupt priority to 6 */
    NVIC->ISER[0] |= 0x00000008;    /* enable IRQ3 for PORTD (D3 of ISER[0]) */
    __enable_irq(); /* global enable IRQs */

S = goN;  
while(1){

LIGHT->DATA = FSM[S].Out;  // set lights
delayMs(FSM[S].Time);
Input = SENSOR->DATA;     // read sensors
Input = Input >> 4;
Input = ~Input;
Input = (Input &= 0x0F);
S = FSM[S].Next[Input];
 
LCD_init();
    {
        //LCD_command(0X80);    /* LCD cursor location */
        delayMs(1);
        LCD_data('W');
        LCD_data('A');
        LCD_data('I');
        LCD_data('T');
    }
  }
}

void GPIOD_Handler(void)
{   int i;
    volatile int readback;
    unsigned char digitPattern[] =
        {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
 
if (S==0)
{
LIGHT->DATA = 0x22;
delayMs(3000);
LIGHT->DATA = 0x24;
}
else if(S == 1)
{
LIGHT->DATA = 0x22;
delayMs(3000);
LIGHT->DATA = 0x24;
}
else if(S == 2)
{
LIGHT->DATA = 0x14;
delayMs(3000);
LIGHT->DATA = 0x24;
}
else if(S == 3)
{
LIGHT->DATA = 0x14;
delayMs(3000);
LIGHT->DATA = 0x24;
}
LCD_init();
LCD_command(1); 
 
     for(i=9;i>=0;i--) {
GPIOB->DATA = digitPattern[i];
delayMs(1000);    
        LCD_command(0X80);    /* LCD cursor location */
        LCD_data('W');
        LCD_data('A');
        LCD_data('L');
        LCD_data('K');
     }


GPIOB->DATA = 0X00;
GPIOD->ICR |= 0x0F;         /* clear the interrupt flag */
readback = GPIOD->ICR;      /* a read to force clearing of interrupt flag */
}

void LCD_init(void)
{
    /* enable clock to GPIOB */
    LCD_PORT->DIR = 0xFF;       /* set all PORTB pins as output */
    LCD_PORT->DEN = 0xFF;       /* set all PORTB pins as digital pins */

    delayMs(20);                /* initialization sequence */
    LCD_nibble_write(0x30, 0);
    delayMs(5);
    LCD_nibble_write(0x30, 0);
    delayUs(100);
    LCD_nibble_write(0x30, 0);
    delayUs(40);

    LCD_nibble_write(0x20, 0);  /* use 4-bit data mode */
    delayUs(40);
    LCD_command(0x28);          /* set 4-bit data, 2-line, 5x7 font */
    LCD_command(0x06);          /* move cursor right */
    LCD_command(0x01);          /* clear screen, move cursor to home */
    LCD_command(0x0F);          /* turn on display, cursor blinking */
}

void LCD_nibble_write(unsigned char data, unsigned char control)
{
    data &= 0xF0;       /* clear lower nibble for control */
    control &= 0x0F;    /* clear upper nibble for data */
    LCD_PORT->DATA = data | control;       /* RS = 0, R/W = 0 */
    LCD_PORT->DATA = data | control | EN;  /* pulse E */
    delayUs(0);
    LCD_PORT->DATA = data;
    LCD_PORT->DATA = 0;
}

void LCD_command(unsigned char command)
{
    LCD_nibble_write(command & 0xF0, 0);   /* upper nibble first */
    LCD_nibble_write(command << 4, 0);     /* then lower nibble */
   
    if (command < 4)
        delayMs(2);         /* commands 1 and 2 need up to 1.64ms */
    else
        delayUs(40);        /* all others 40 us */
}

void LCD_data(unsigned char data)
{
    LCD_nibble_write(data & 0xF0, RS);    /* upper nibble first */
    LCD_nibble_write(data << 4, RS);      /* then lower nibble  */
   
    delayUs(40);
}

/* delay n milliseconds (16 MHz CPU clock) */
/* delay n microseconds (16 MHz CPU clock) */

void delayUs(int n)
{
    int i, j;
    for(i = 0 ; i < n; i++)
        for(j = 0; j < 3; j++)
            {}  /* do nothing for 1 us */
}
void delayMs(int n)
{
    int i, j;
    for(i = 0 ; i < n; i++)
        for(j = 0; j < 3180; j++)
            {}  /* do nothing for 1 ms */
}

void SystemInit(void)
{

    /* Grant coprocessor access*/
    /* This is required since TM4C123G has a floating point coprocessor */
    SCB->CPACR |= 0x00F00000;
} 
