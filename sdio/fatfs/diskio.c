/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sdio.h"		/* SD card driver        */

/* Definitions of physical drive number for each drive */
#define SDHC            0
#define SECTOR_SIZE     512  /* Block Size in Bytes */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
		BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv == SDHC)
		return STA_OK;
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
		BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv == SDHC)
	{
		if(SD_Init() == SD_OK)
			return STA_OK;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
		BYTE pdrv,		/* Physical drive nmuber to identify the drive */
		BYTE *buff,		/* Data buffer to store read data */
		DWORD sector,	/* Sector address in LBA */
		UINT count		/* Number of sectors to read */
)
{
	if(pdrv != SDHC)
		return RES_PARERR;
	if(count == 0)
		return RES_PARERR;
	SD_Error res = SD_OK;
	if(count==1)
		res = SD_ReadBlock(buff, sector * SECTOR_SIZE, SECTOR_SIZE);
	else
		res = SD_ReadMultiBlocks(buff, sector * SECTOR_SIZE, SECTOR_SIZE, count);

	if(res == SD_OK)
		return RES_OK;

	return RES_ERROR;

}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
		BYTE pdrv,			/* Physical drive nmuber to identify the drive */
		const BYTE *buff,	/* Data to be written */
		DWORD sector,		/* Sector address in LBA */
		UINT count			/* Number of sectors to write */
)
{
	if(pdrv != SDHC)
		return RES_PARERR;
	if(count == 0)
		return RES_PARERR;

	SD_Error res = SD_OK;
	if(count==1)
		res = SD_WriteBlock((uint8_t *)buff, sector * SECTOR_SIZE, SECTOR_SIZE);
	else
		res = SD_WriteMultiBlocks((uint8_t *)buff, sector * SECTOR_SIZE, SECTOR_SIZE, count);

	if(res == SD_OK)
		return RES_OK;

	return RES_ERROR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
		BYTE pdrv,		/* Physical drive nmuber (0..) */
		BYTE cmd,		/* Control code */
		void *buff		/* Buffer to send/receive control data */
)
{
	if(pdrv != SDHC)
		return RES_PARERR;

	return RES_OK;
}
#endif


DWORD get_fattime (void)
{
	return 0;
}
