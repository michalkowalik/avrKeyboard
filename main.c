/*
  Michal Kowalik, 2016
 */
#include "sun_defs.h"
#include "keycodes.h"
#include "utils.h"

static int newUsartByte = 0;
static report_keyboard keyReportBuffer;
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
          if(rq -> wValue.bytes[0] == 1)
            {
              usbMsgPtr = &keyReportBuffer;
              return sizeof(keyReportBuffer);
            } 
          else 
            //no such descriptor:
            return 0;
          
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

  // default:
  return 0;
}

// build USB report buffer - 
// based on the array in keycodes.h
static uchar buildUsbReport(uchar rb) 
{

  // 0: key down, key up otherwise
  uchar keyUp = rb & 0x80;
  uchar cnt;

  // Set report ID: 
  keyReportBuffer.id = 1;

  uchar usbKey = pgm_read_byte(&(sunkeycodes[rb & 0x7f]));

  if(usbKey == 0)
    return 0;

  // check if macro key:
  // TODO

  // check modifier 
  if ((usbKey >= 0xE0) && (usbKey <= 0xE7)) 
    {
      if (keyUp)
        {
          keyReportBuffer.modifier &= ~(1 << (usbKey - 0xE0));
        }
      else 
        {
          keyReportBuffer.modifier |= (1 << (usbKey - 0xE0)) ;
        }
    }

  // check normal keys:
  if (keyUp) 
    {
      for (cnt = 0; cnt < sizeof keyReportBuffer.keycode; ++cnt)
        {
          if (keyReportBuffer.keycode[cnt] == usbKey)
            {
              keyReportBuffer.keycode[cnt] = 0;
              break;
            }
        }
    }
  else 
    {
      for (cnt = 0; cnt < sizeof keyReportBuffer.keycode; ++cnt)
        {
          if (keyReportBuffer.keycode[cnt] == 0)
            {
              keyReportBuffer.keycode[cnt] = usbKey;
              break;
            }
        }
    }

  // key was pressed
  return 1;
}

// TODO:
// Check if it may be the part of the problem?
// there's no ID anywhere here
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


// Interrupt handling:
// Process bytes coming from the keyboard.
ISR(USART_RXC_vect)
{
  char receivedByte;
    
  // Fetch the recieved byte value into the variable:
  receivedByte = UDR;
  
  if (buildUsbReport(receivedByte))
    newUsartByte = 1;
}


// guarantee the data is sent to computer only once:
// unused so far!
void usbSendHidReport(uchar *data, uchar len)
{
  while(1)
    {
      usbPoll();
      if(usbInterruptIsReady())
        {
          usbSetInterrupt(data, len);
          break;
        }
    }
}


int main() 
{
  uint8_t updateNeeded = 0;
  uchar i;

  uchar idleCounter = 0;

  // manually set report ID:
  keyReportBuffer.id = 1;

  // zeros in the report buffer:
  for (i = 0; i < sizeof keyReportBuffer.keycode; ++i) 
      keyReportBuffer.keycode[i] = 0;

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

  blinkB2();

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
        usbSetInterrupt((void *)&keyReportBuffer, sizeof keyReportBuffer);
      }
  }

  return 0;
}
