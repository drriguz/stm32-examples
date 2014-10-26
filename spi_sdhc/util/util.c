#include "util.h"

volatile u16 _timerDelay;
void delay_ms(u16 ms)
{
	_timerDelay = ms;

	while(_timerDelay);
}

void SysTick_Handler(void)
{
	if(_timerDelay)
		_timerDelay --;
}
