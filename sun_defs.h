#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define F_CPU 16000000L

#include "usbdrv.h"
#include <util/delay.h>


// function definitions:
// static int usartInit();
// static void uart_putchar(uchar c);
// static uchar buildUsbReport(uchar rb);
usbMsgLen_t usbFunctionSetup(uchar data[8]);
usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len);
