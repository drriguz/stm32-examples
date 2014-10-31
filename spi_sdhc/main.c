/**
 * SPI 1 of STM32F103ZET6
 * PA4 NSS  ->SD_CS
 * PA5 SCK  ->SD_CLK
 * PA6 MISO ->SD_OUT
 * PA7 MOSI ->SD_IN
 */

#include "stm32f10x_conf.h"
#include "util.h"
#include "sdcard.h"
#include "diskio.h"
#include "ff.h"

#define NULL 0

FATFS       fatfs;
FIL         fsrc, fdst;
BYTE        buffer[512];

void Led_GPIO_Configuration(void);
int main(void)
{

	SystemInit();
	if(SysTick_Config(SystemCoreClock / 1000))
	{
		//capture error
		while(1);
	}
	Led_GPIO_Configuration();
	initSPI();

	DRESULT res = disk_initialize(0);
	res = f_mount(&fatfs, "0", 0);
	res = f_open(&fsrc, "0:/hello.txt",  FA_READ|FA_WRITE|FA_OPEN_EXISTING);
	UINT cnt = 0;
	res = f_read(&fsrc, buffer, 10, &cnt);
	res = f_lseek(&fsrc, 1);
	cnt = f_puts("\r\nI Love Stm32^_^\r\n", &fsrc);
	f_sync(&fsrc);

	res = f_open(&fdst, "0:/new.txt", FA_CREATE_NEW|FA_WRITE);
	BYTE textFileBuffer[] = "hello world\r\n";
	UINT bw = 0;
	res = f_write(&fdst, textFileBuffer, sizeof(textFileBuffer), &bw);
	f_mount(NULL, 0, 0);
	while(1)
	{
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		delay_ms(1000);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_ms(2000);

	}
}

void Led_GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC , ENABLE);

	//LED -> PC13
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}
