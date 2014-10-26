#ifndef _SOLEE_SD_CARD_SPI_H_
#define _SOLEE_SD_CARD_SPI_H_

#include "stm32f10x_conf.h"

// SD卡类型定义
#define SD_TYPE_MMC     0
#define SD_TYPE_V1      1
#define SD_TYPE_V2      2
#define SD_TYPE_V2HC    4


// SPI总线速度设置
#define SPI_SPEED_LOW   0
#define SPI_SPEED_HIGH  1

#define MSD_BLOCKSIZE   512

/* SD/MMC command list - SPI mode */
#define CMD0                     0       /* Reset */
#define CMD1                     1       /* Send Operator Condition - SEND_OP_COND */
#define CMD8                     8       /* Send Interface Condition - SEND_IF_COND	*/
#define CMD9                     9       /* Read CSD */
#define CMD10                    10      /* Read CID */
#define CMD12                    12      /* Stop data transmit */
#define CMD16                    16      /* Set block size, should return 0x00 */
#define CMD17                    17      /* Read single block */
#define CMD18                    18      /* Read multi block */
#define ACMD23                   23      /* Prepare erase N-blokcs before multi block write */
#define CMD24                    24      /* Write single block */
#define CMD25                    25      /* Write multi block */
#define ACMD41                   41      /* should return 0x00 */
#define CMD55                    55      /* should return 0x01 */
#define CMD58                    58      /* Read OCR */
#define CMD59                    59      /* CRC disable/enbale, should return 0x00 */

typedef enum
{
	/**
	 * @brief  SD reponses and error flags
	 */
	SD_RESPONSE_NO_ERROR      = (0x00),
	SD_IN_IDLE_STATE          = (0x01),
	SD_ERASE_RESET            = (0x02),
	SD_ILLEGAL_COMMAND        = (0x04),
	SD_COM_CRC_ERROR          = (0x08),
	SD_ERASE_SEQUENCE_ERROR   = (0x10),
	SD_ADDRESS_ERROR          = (0x20),
	SD_PARAMETER_ERROR        = (0x40),
	SD_RESPONSE_FAILURE       = (0xFF),

	/**
	 * @brief  Data response error
	 */
	SD_DATA_OK                = (0x05),
	SD_DATA_CRC_ERROR         = (0x0B),
	SD_DATA_WRITE_ERROR       = (0x0D),
	SD_DATA_OTHER_ERROR       = (0xFF)
} SD_Error;

typedef struct               /* Card Specific Data */
{
	vu8  CSDStruct;            /* CSD structure */
	vu8  SysSpecVersion;       /* System specification version */
	vu8  Reserved1;            /* Reserved */
	vu8  TAAC;                 /* Data read access-time 1 */
	vu8  NSAC;                 /* Data read access-time 2 in CLK cycles */
	vu8  MaxBusClkFrec;        /* Max. bus clock frequency */
	vu16 CardComdClasses;      /* Card command classes */
	vu8  RdBlockLen;           /* Max. read data block length */
	vu8  PartBlockRead;        /* Partial blocks for read allowed */
	vu8  WrBlockMisalign;      /* Write block misalignment */
	vu8  RdBlockMisalign;      /* Read block misalignment */
	vu8  DSRImpl;              /* DSR implemented */
	vu8  Reserved2;            /* Reserved */
	vu32 DeviceSize;           /* Device Size */
	vu8  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
	vu8  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
	vu8  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
	vu8  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
	vu8  DeviceSizeMul;        /* Device size multiplier */
	vu8  EraseGrSize;          /* Erase group size */
	vu8  EraseGrMul;           /* Erase group size multiplier */
	vu8  WrProtectGrSize;      /* Write protect group size */
	vu8  WrProtectGrEnable;    /* Write protect group enable */
	vu8  ManDeflECC;           /* Manufacturer default ECC */
	vu8  WrSpeedFact;          /* Write speed factor */
	vu8  MaxWrBlockLen;        /* Max. write data block length */
	vu8  WriteBlockPaPartial;  /* Partial blocks for write allowed */
	vu8  Reserved3;            /* Reserded */
	vu8  ContentProtectAppli;  /* Content protection application */
	vu8  FileFormatGrouop;     /* File format group */
	vu8  CopyFlag;             /* Copy flag (OTP) */
	vu8  PermWrProtect;        /* Permanent write protection */
	vu8  TempWrProtect;        /* Temporary write protection */
	vu8  FileFormat;           /* File Format */
	vu8  ECC;                  /* ECC code */
	vu8  CSD_CRC;              /* CSD CRC */
	vu8  Reserved4;            /* always 1*/
}MSD_CSD;

typedef struct				 /*Card Identification Data*/
{
	vu8  ManufacturerID;       /* ManufacturerID */
	vu16 OEM_AppliID;          /* OEM/Application ID */
	vu32 ProdName1;            /* Product Name part1 */
	vu8  ProdName2;            /* Product Name part2*/
	vu8  ProdRev;              /* Product Revision */
	vu32 ProdSN;               /* Product Serial Number */
	vu8  Reserved1;            /* Reserved1 */
	vu16 ManufactDate;         /* Manufacturing Date */
	vu8  CID_CRC;              /* CID CRC */
	vu8  Reserved2;            /* always 1 */
}MSD_CID;

typedef struct
{
	MSD_CSD CSD;
	MSD_CID CID;
	u32 Capacity;              /* Card Capacity */
	u32 BlockSize;             /* Card Block Size */
	u16 RCA;
	u8 CardType;
	u32 SpaceTotal;            /* Total space size in file system */
	u32 SpaceFree;      	   /* Free space size in file system */
}MSD_CARDINFO, *PMSD_CARDINFO;

#define CARDTYPE_MMC     	     0x00
#define CARDTYPE_SDV1      	     0x01
#define CARDTYPE_SDV2      	     0x02
#define CARDTYPE_SDV2HC    	     0x04

#define DUMMY_BYTE	             0xFF

#define SD_CS_ENABLE()      GPIO_ResetBits(GPIOA, GPIO_Pin_4)   //选中SD卡
#define SD_CS_DISABLE()     GPIO_SetBits(GPIOA, GPIO_Pin_4)     //不选中SD卡

#define SD_NO_CARD           0x99
#define SD_WRITE_PROTECTED   0x98
#define SD_TIMEOUT           0x89
#define SD_RECEIVE_ERROR     0x79
#define SD_ERROR_RESPONCE    0x69




void initSPI();

int sdDetect();			//检测SD是否插入
void sdPowerOn();

u8 spiReadWrite(u8 data);
void spiSetSpeed(u8 SpeedSet);
u8 sdWaitReady(void);                            //等待SD卡就绪
u8 sdSendCommand(u8 cmd, u32 arg, u8 crc);       //SD卡发送一个命令
u8 sdSendCommandNoDeassert(u8 cmd, u32 arg, u8 crc);       //SD卡发送一个命令
u8 sdInit(void);                                 //SD卡初始化
u8 sdReceiveData(u8 *data, u16 len, u8 release); //SD卡读数据
//u8 sdGetCID(u8 *cid_data);                       //读SD卡CID
//u8 sdGetCSD(u8 *csd_data);                       //读SD卡CSD
int sdGetCardInfo(PMSD_CARDINFO cardInfo);
//u32 sdGetCapacity(void);                         //取SD卡容量

u8 sdReadSingleBlock(u32 sector, u8 *buffer);                //读一个sector
u8 sdWriteSingleBlock(u32 sector, const u8 *buffer);         //写一个sector
u8 sdReadMultiBlock(u32 sector, u8 *buffer, u8 count);       //读多个sector
u8 sdWriteMultiBlock(u32 sector, const u8 *data, u8 count);  //写多个sector

#endif
