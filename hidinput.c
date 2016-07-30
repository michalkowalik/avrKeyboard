#include "hidinput.h"
#include "avr.h"

static uchar reportKeyboard()
{
  uchar i, r = 2;
  ReportBuffer[0] = IDKeyboard;
  ReportBuffer[1] = 0; // modifiers:

}
