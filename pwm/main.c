#include "stm32f10x_conf.h"
#include "pwm.h"

int main(void)
{
	SystemInit();
	initTim3Pwm();
	while(1)
	{
	}
}
