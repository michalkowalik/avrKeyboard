#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define F_CPU 16000000L

#include "usbdrv.h"
#include <util/delay.h>

#include "keycodes.h"
#include "sun_defs.h"

#define NUMLOCK 1
#define CAPSLOCK 2
#define SCROLLLOCK 4

typedef struct {
  uint8_t modifier;
  uint8_t reserved;
  uint8_t keycode[6];
} report_t;

static report_t reportBuffer;
volatile static uchar LED_state = 0xff;

// repeat rate for keyboards
static uchar idleRate;

// send byte to keyb:
static void uart_putchar(uchar c)
{
	loop_until_bit_is_set(UCSRA, UDRE);
    UDR = c;
}


usbMsgLen_t usbFunctionSetup(uchar data[8]) 
{
  usbRequest_t *rq = (void *)data;

  if((rq -> bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {
      switch(rq -> bRequest) 
        {
          // send "no keys pressed" if asked here
        case USBRQ_HID_GET_REPORT:
          usbMsgPtr = (void *)&reportBuffer;
          reportBuffer.modifier = 0;
          reportBuffer.keycode[0] = 0;
          return sizeof(reportBuffer);

          // if wLength == 1 -> led state
        case USBRQ_HID_SET_REPORT:
          return (rq -> wLength.word == 1) ? USB_NO_MSG : 0;

          // set idle rate:
        case USBRQ_HID_GET_IDLE:
          usbMsgPtr = &idleRate;
          return 1;

          // save idle rate as required by spec:
        case USBRQ_HID_SET_IDLE:
          idleRate = rq -> wValue.bytes[1];
          return 0;
        }
    }

  // by default don't return any data:
  return 0;
}

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len)
{
  uchar cLED = 0;

  if(data[0] == LED_state)
    return 1;
  else
    LED_state = data[0];

  if (LED_state & USB_LED_NLOCK) {
    cLED |= 0x01;
  }
  if (LED_state & USB_LED_CLOCK) {
    cLED |= 0x08;
  }
  if (LED_state & USB_LED_SCRLCK) {
    cLED |= 0x04;
  }
  if (LED_state & USB_LED_CMPOSE) {
    cLED |= 0x02;
  }
  uart_putchar(SKBDCMD_SETLED);
  uart_putchar(cLED);


  // LED state changed: - DEV board only!
  if(LED_state & NUMLOCK)
    {
      // LED ON
      PORTB |= 1 << PB0;
    } else 
    {
      // LED OFF
      PORTB &= ~(1 << PB0);
  }
  return 1;
}

static int usartInit()
{
  // Turn on the transmission and reception circuitry
   UCSRB |= (1 << RXEN) | (1 << TXEN);
   // Use 8-bit character sizes
   UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);
   // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
   UBRRL = BAUD_PRESCALE; 
   // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
   UBRRH = (BAUD_PRESCALE >> 8);
   // Enable the USART Recieve Complete interrupt (USART_RXC)  (( RXCIE ? RCXIE  ))
   UCSRB |= (1 << RXCIE); 
 
  return 0;
}

void blinkB1()
{
  PORTB |= 1 << PB1;
  _delay_ms(50);
  PORTB &= ~(1 << PB1);
}

void blinkB2()
{
  PORTB |= 1 << PB2;
  _delay_ms(50);
  PORTB &= ~(1 << PB2);
}

// Interrupt handling:

// debug only, blink LED on TX:
ISR(USART_TXC_vect)
{
  blinkB2();
}


// Process bytes coming from the keyboard.
ISR(USART_RXC_vect)
{
  char ReceivedByte;
  char sendByte[2];
  
  // Fetch the recieved byte value into the variable "ByteReceived"
  ReceivedByte = UDR;
  
  // blink LED on port B1:
  blinkB1();
}

int main() 
{
  uchar i;

  // enable 1 sec watchdog timer:
  wdt_enable(WDTO_1S);

  // Port B as output:
  DDRB = 0xFF;

  usartInit();
  _delay_ms(100);
  usbInit();

  // force re-enumeration:
  usbDeviceDisconnect();
  for(i = 0; i < 250; i++) {
    wdt_reset();
    _delay_ms(2);
  }

  usbDeviceConnect();

  // enable interrupts:
  sei();

  while(1) {
    wdt_reset();
    usbPoll();
    /*
    if(usbInterruptIsReady()) {
      rand = (rand * 109 + 89) % 251;

      reportBuffer.dx = (rand&0xf) - 8;
      reportBuffer.dy = ((rand&0xf0) >> 4) - 8;

      usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
    }
    */
  }

  return 0;
}
