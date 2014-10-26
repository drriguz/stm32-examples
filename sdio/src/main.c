#include "stm32f10x.h"
#include "sdio.h"
#include "diskio.h"
#include "ff.h"

#define NULL 0

SD_CardInfo sdcardInfo;
FATFS       fatfs;
FIL         fsrc, fdst;
BYTE        buffer[512];

int main(void)
{
	SystemInit();

	DRESULT res = disk_initialize(0);
	f_mount(&fatfs, 0, 0);
	res = f_open(&fsrc, "0:/hello.txt", FA_READ|FA_OPEN_EXISTING);

	f_mount(NULL, 0, 0);
	while(1)
	{

	}
}
