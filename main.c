/*
  Michal Kowalik, 2016
 */

#include "sun_defs.h"
#include "keycodes.h"


static int newUsartByte = 0;
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
  uchar reportType;

  if((rq -> bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {
      switch(rq -> bRequest) 
        {
          // send "no keys pressed" if asked here
        case USBRQ_HID_GET_REPORT:
          reportType = rq->wValue.bytes[0];
          usbMsgPtr = ReportBuffer;

          // report Type: - TODO: double check if it will work!:
          ReportBuffer[0] = rq->wValue.bytes[0];

          // modifiers:
          ReportBuffer[1] = 0;

          // empty key buffer:
          ReportBuffer[2] = 0;
          return sizeof(ReportBuffer);

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

// build USB report buffer - 
// based on the array in keycodes.h
static uchar buildUsbReport(uchar rb) 
{

  // 0: key down, key up otherwise
  uchar keyUp = rb & 0x80;
  uchar cnt;

  uchar usbKey = pgm_read_byte(&(sunkeycodes[rb & 0x7f]));

  if(usbKey == 0)
    return 0;

  // Report ID:
  ReportBuffer[0] = ID_Keyboard;
  // check modifier keys:
  if ((usbKey >= 0xE0) && (usbKey <= 0xE7)) 
    {
      if (keyUp)
        {
          ReportBuffer[1] &= ~(1 << (usbKey - 0xE0));
        }
      else {
        ReportBuffer[1] |= (1 << (usbKey -0xE0)) ;
      }
    }

  // check normal keys:
  if (keyUp) 
    {
      for (cnt = 2; cnt < sizeof ReportBuffer; ++cnt)
        {
          if (ReportBuffer[cnt] == usbKey)
            {
              ReportBuffer[cnt] = 0;
              break;
            }
        }
    }
  else 
    {
      for (cnt = 2; cnt < sizeof ReportBuffer; ++cnt)
        {
          if (ReportBuffer[cnt] == 0)
            {
              ReportBuffer[cnt] = usbKey;
              break;
            }
        }
    }

  // key was pressed
  return 1;
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

// Interrupt handling:
// Process bytes coming from the keyboard.
ISR(USART_RXC_vect)
{
  char receivedByte;
  
  // Fetch the recieved byte value into the variable "ByteReceived"
  receivedByte = UDR;
  
  if (buildUsbReport(receivedByte))
    newUsartByte = 1;
}

int main() 
{
  uint8_t updateNeeded = 0;
  uchar i;

  uchar idleCounter = 0;

  // zeros in the report buffer:
  for (i = 2; i < sizeof ReportBuffer; ++i) 
      ReportBuffer[i] = 0;

  // enable 1 sec watchdog timer:
  wdt_enable(WDTO_1S);

  // Port B as output:
  DDRB = 0xFF;

  usartInit();
  _delay_ms(100);
  usbInit();

  TCCR0 = 5;      /* timer 0 prescaler: 1024 */

  // force re-enumeration:
  usbDeviceDisconnect();
  for(i = 0; i < 250; i++) {
    wdt_reset();
    _delay_ms(2);
  }

  usbDeviceConnect();

  blinkB1();

  // enable interrupts:
  sei();

  while(1) {
    wdt_reset();
    usbPoll();

    // do I really need it?
    updateNeeded = newUsartByte;


    // check timer if we need periodic reports
    if (TIFR & (1 << TOV0))
      {
        TIFR = (1 << TOV0); // reset flag
        if (idleRate != 0)
          { // do we need periodic reports?
            if(idleCounter > 4){ // yes, but not yet
              idleCounter -= 5; // 22ms in units of 4ms
            } 
            else 
              { // yes, it is time now
                updateNeeded = 1;
                idleCounter = idleRate;
              }
          }
      }

    if (updateNeeded && usbInterruptIsReady())
      {
        updateNeeded = 0;
        newUsartByte = 0;
        usbSetInterrupt(ReportBuffer, sizeof ReportBuffer);
      }
  }

  return 0;
}
