
#include "sun_defs.h"
#include "utils.h"

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

void blinkB3()
{
  PORTB |= 1 << PB3;
  _delay_ms(50);
  PORTB &= ~(1 << PB3);
}
