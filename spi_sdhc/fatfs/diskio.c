/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sdcard.h"		/* SD card driver        */

/* Definitions of physical drive number for each drive */
#define SDHC            0
#define SECTOR_SIZE     512  /* Block Size in Bytes */

extern MSD_CARDINFO cardInfo;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
		BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv == SDHC)
		return STA_OK;
	if(!sdDetect())
		return STA_NODISK;
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
		if(sdInit() == 0)
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
	if(!sdDetect())
		return RES_NOTRDY;
	SD_Error res = 0;
	if(count==1)
		res = sdReadSingleBlock(sector, buff);
	else
		res = sdReadMultiBlock(sector, buff, count);

	if(res == 0)
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
	if(!sdDetect())
		return RES_NOTRDY;

	SD_Error res = 0;
	if(count==1)
		res = sdWriteSingleBlock(sector, buff);
	else
		res = sdWriteMultiBlock(sector, buff, count);

	if(res == 0)
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
	DRESULT res;
	switch(cmd)
	{
	case CTRL_SYNC:
		SD_CS_ENABLE();
		if(sdWaitReady())
			res = RES_OK;
		else
			res = RES_ERROR;
		SD_CS_DISABLE();
		break;
	case GET_BLOCK_SIZE:
		*(WORD*)buff = MSD_BLOCKSIZE;
		res = RES_OK;
		break;
	case GET_SECTOR_COUNT:
		*(DWORD*)buff = cardInfo.Capacity;
		res = RES_OK;
		break;
	default:
		res = RES_PARERR;
		break;
	}
	return res;
}
#endif


DWORD get_fattime (void)
{
	return 0;
}
