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


usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t *rq = (void *)data;

  // The following requests are never used. But since they are required by
  // the specification, we implement them in this example.
  if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {

    if(rq->bRequest == USBRQ_HID_GET_REPORT) {  
      // wValue: ReportType (highbyte), ReportID (lowbyte)
      usbMsgPtr = (void *)&reportBuffer; // we only have this one
      return sizeof(reportBuffer);
    } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
      usbMsgPtr = &idleRate;
      return 1;
    } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
      idleRate = rq->wValue.bytes[1];
    }
  }
  return 0; // by default don't return any data
}

/* -- initial implementation of the usbFunctionSetup
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
  return 0; // do nothing for now
}
*/


int main() {
  uchar i;
  int rand = 1234;

  // enable 1 sec watchdog timer:
  wdt_enable(WDTO_1S);

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

    if(usbInterruptIsReady()) {
      rand = (rand * 109 + 89) % 251;

      reportBuffer.dx = (rand&0xf) - 8;
      reportBuffer.dy = ((rand&0xf0) >> 4) - 8;

      usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
    }

  }

  return 0;
}
