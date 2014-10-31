#include "stm32f10x_conf.h"
#include "pwm.h"

static void TIM3_GPIO_Config(void)
{
	GPIO_InitTypeDef gpioType;

	/* TIM3 clock enable */
	//PCLK1经过2倍频后作为TIM3的时钟源等于36MHz
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* GPIOA and GPIOB clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

	/*GPIOA Configuration: TIM3 channel 1 and 2 as alternate function push-pull */
	gpioType.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
	gpioType.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioType.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &gpioType);

	/*GPIOB Configuration: TIM3 channel 3 and 4 as alternate function push-pull */
	gpioType.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1;

	GPIO_Init(GPIOB, &gpioType);
}


static void TIM3_Mode_Config(void)
{
	TIM_TimeBaseInitTypeDef  timeType;
	TIM_OCInitTypeDef  ocType;

	/* PWM信号电平跳变值 */
	u16 CCR1_Val = 500;
	u16 CCR2_Val = 375;
	u16 CCR3_Val = 250;
	u16 CCR4_Val = 125;

	/* -----------------------------------------------------------------------
    TIM3 Configuration: generate 4 PWM signals with 4 different duty cycles:
    TIM3CLK = 36 MHz, Prescaler = 0x0, TIM3 counter clock = 36 MHz
    TIM3 ARR Register = 999 => TIM3 Frequency = TIM3 counter clock/(ARR + 1)
    TIM3 Frequency = 36 KHz.
    TIM3 Channel1 duty cycle = (TIM3_CCR1/ TIM3_ARR)* 100 = 50%
    TIM3 Channel2 duty cycle = (TIM3_CCR2/ TIM3_ARR)* 100 = 37.5%
    TIM3 Channel3 duty cycle = (TIM3_CCR3/ TIM3_ARR)* 100 = 25%
    TIM3 Channel4 duty cycle = (TIM3_CCR4/ TIM3_ARR)* 100 = 12.5%
  ----------------------------------------------------------------------- */

	/* Time base configuration */
	timeType.TIM_Period = 999;       //当定时器从0计数到999，即为1000次，为一个定时周期
	timeType.TIM_Prescaler = 0;	    //设置预分频：不预分频，即为36MHz
	timeType.TIM_ClockDivision = 0;	//设置时钟分频系数：不分频
	timeType.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数模式

	TIM_TimeBaseInit(TIM3, &timeType);

	/* PWM1 Mode configuration: Channel1 */
	ocType.TIM_OCMode = TIM_OCMode_PWM1;	    //配置为PWM模式1
	ocType.TIM_OutputState = TIM_OutputState_Enable;
	ocType.TIM_Pulse = CCR1_Val;	   //设置跳变值，当计数器计数到这个值时，电平发生跳变
	ocType.TIM_OCPolarity = TIM_OCPolarity_High;  //当定时器计数值小于CCR1_Val时为高电平

	TIM_OC1Init(TIM3, &ocType);	 //使能通道1

	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	/* PWM1 Mode configuration: Channel2 */
	ocType.TIM_OutputState = TIM_OutputState_Enable;
	ocType.TIM_Pulse = CCR2_Val;	  //设置通道2的电平跳变值，输出另外一个占空比的PWM

	TIM_OC2Init(TIM3, &ocType);	  //使能通道2

	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

	/* PWM1 Mode configuration: Channel3 */
	ocType.TIM_OutputState = TIM_OutputState_Enable;
	ocType.TIM_Pulse = CCR3_Val;	//设置通道3的电平跳变值，输出另外一个占空比的PWM

	TIM_OC3Init(TIM3, &ocType);	 //使能通道3

	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

	/* PWM1 Mode configuration: Channel4 */
	ocType.TIM_OutputState = TIM_OutputState_Enable;
	ocType.TIM_Pulse = CCR4_Val;	//设置通道4的电平跳变值，输出另外一个占空比的PWM

	TIM_OC4Init(TIM3, &ocType);	//使能通道4

	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM3, ENABLE);			 // 使能TIM3重载寄存器ARR

	/* TIM3 enable counter */
	TIM_Cmd(TIM3, ENABLE);                   //使能定时器3
}

void initTim3Pwm()
{
	TIM3_GPIO_Config();
	TIM3_Mode_Config();
}
