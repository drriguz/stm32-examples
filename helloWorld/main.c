#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "util.h"

void ledConfig();

int main(void)
{
	SystemInit();

	//set system tick to 1s
	SysTick_Config(SystemCoreClock / 1000);

	//initialize GPIO setting for led
	ledConfig();

	//main loop
	while(1)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_ms(1000);
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		delay_ms(200);
	}
}

void ledConfig()
{
	GPIO_InitTypeDef gpioType;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC , ENABLE);

	//LED -> PC13
	gpioType.GPIO_Pin = GPIO_Pin_13;
	gpioType.GPIO_Speed = GPIO_Speed_50MHz;
	gpioType.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpioType);
}
