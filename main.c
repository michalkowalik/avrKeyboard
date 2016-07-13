#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define F_CPU 16000000L

#include "usbdrv.h"
#include <util/delay.h>

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

/* USB report descriptor, size must match usbconfig.h */
PROGMEM const char usbHidReportDescriptor[63] = { 
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION     
};


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
  if(data[0] == LED_state)
    return 1;
  else
    LED_state = data[0];

  // LED state changed:
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


int main() 
{
  uchar i;

  // enable 1 sec watchdog timer:
  wdt_enable(WDTO_1S);

  // PB0 as output:
  DDRB = 1 << PB0;

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
