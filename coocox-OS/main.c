#include <CoOS.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define TASK_STACK_SIZE           128

OS_STK led_blue_stack[TASK_STACK_SIZE];
OS_STK led_red_stack[TASK_STACK_SIZE];
OS_STK led_yellow_stack[TASK_STACK_SIZE];

OS_MutexID ry_mutex;
OS_FlagID  r_flag;
OS_FlagID  y_flag;

#define BLUE_ON     GPIO_SetBits(GPIOF, GPIO_Pin_0)
#define RED_ON   GPIO_SetBits(GPIOF, GPIO_Pin_1)
#define YELLOW_ON  GPIO_SetBits(GPIOF, GPIO_Pin_2)

#define BLUE_OFF    GPIO_ResetBits(GPIOF, GPIO_Pin_0)
#define RED_OFF  GPIO_ResetBits(GPIOF, GPIO_Pin_1)
#define YELLOW_OFF GPIO_ResetBits(GPIOF, GPIO_Pin_2)

volatile unsigned int cnt = 0;

void initGPIO()
{
	GPIO_InitTypeDef gpioType;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOF , ENABLE);

	//LED -> PC13
	gpioType.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	gpioType.GPIO_Speed = GPIO_Speed_50MHz;
	gpioType.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOF, &gpioType);
}

void redLed(void *pdata)
{
	ry_mutex = CoCreateMutex();
	if(ry_mutex == E_CREATE_FAIL)
	{

	}
	r_flag = CoCreateFlag(Co_TRUE, 0);
	if(r_flag == E_CREATE_FAIL)
	{

	}
	y_flag = CoCreateFlag(Co_TRUE, 0);
	if(y_flag == E_CREATE_FAIL)
	{

	}
	while(1)
	{
		CoWaitForSingleFlag(r_flag, 0);
		CoEnterMutexSection(ry_mutex);
		RED_ON;
		CoTickDelay(50);
		RED_OFF;
		CoTickDelay(10);
		CoLeaveMutexSection(ry_mutex);
	}
}

void yellowLed(void *pdata)
{
	while(1)
	{
		CoWaitForSingleFlag(y_flag, 0);
		CoEnterMutexSection(ry_mutex);
		YELLOW_ON;
		CoTickDelay(100);
		YELLOW_OFF;
		CoTickDelay(10);
		CoLeaveMutexSection(ry_mutex);

	}
}

void blueLed(void *pdata)
{
	while(1)
	{
		BLUE_ON;
		CoTickDelay(5);
		BLUE_OFF;
		CoTickDelay(20);
		if(cnt%2 == 0)
		{
			CoSetFlag(r_flag);
		}
		else
		{
			CoSetFlag(y_flag);
		}
		cnt ++;
		if(cnt > 9999)
			cnt = 1;
	}
}


int main(void)
{
	initGPIO();
	CoInitOS();
	CoCreateTask(blueLed, 0, 0, &led_blue_stack[TASK_STACK_SIZE - 1], TASK_STACK_SIZE);
	CoCreateTask(redLed, 0, 0, &led_red_stack[TASK_STACK_SIZE - 1], TASK_STACK_SIZE);
	CoCreateTask(yellowLed, 0, 0, &led_yellow_stack[TASK_STACK_SIZE - 1], TASK_STACK_SIZE);
	CoStartOS();
	while(1);
}
