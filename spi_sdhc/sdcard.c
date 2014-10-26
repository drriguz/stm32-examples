#include "stm32f10x_conf.h"
#include "sdcard.h"

MSD_CARDINFO cardInfo;


/**
 * 检测SD卡是否插入
 */
int sdDetect()
{
	return 1;
}

void sdPowerOn()
{
	return;
}


void initSPI()
{
	GPIO_InitTypeDef gpioTyp;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA| RCC_APB2Periph_SPI1, ENABLE);


	//SPI1_SCK -> PA5 , SPI1_MISO -> PA6 , SPI1_MOSI ->	PA7
	gpioTyp.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	gpioTyp.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioTyp.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioTyp);

	//SD_CS -> PA4
	//NSS 初始化为推挽输出
	gpioTyp.GPIO_Pin = GPIO_Pin_4;
	gpioTyp.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpioTyp);

	//停止SPI
	SPI_Cmd(SPI1, DISABLE);

	spiSetSpeed(SPI_SPEED_LOW);
	//使能SPI外设
	SPI_Cmd(SPI1, ENABLE);
}

/**
 * SPI读写一个字节（发送完成后返回本次通讯读取的数据）
 */
u8 spiReadWrite(u8 data)
{
	/* Loop while DR register in not emplty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

	/* Send Half Word through the SPI1 peripheral */
	SPI_I2S_SendData(SPI1, data);

	/* Wait to receive a Half Word */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
	//while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

	/* Return the Half Word read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI1);
}

/**
 * SPI初始化，指定低速或者是高速模式
 * SPI_SPEED_HIGH   1
 * SPI_SPEED_LOW    0
 */
void spiSetSpeed(u8 SpeedSet)
{
	SPI_InitTypeDef  spiTyp;
	//SPI1 设置
	spiTyp.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   //SPI为双线双向全双工
	spiTyp.SPI_Mode = SPI_Mode_Master;                        //SPI工作模式：主SPI
	spiTyp.SPI_DataSize = SPI_DataSize_8b;                    //SPI数据大小：SPI发送接收8位帧结构
	spiTyp.SPI_CPOL = SPI_CPOL_High;                          //选择串行时钟的稳态：时钟悬空高x
	spiTyp.SPI_CPHA = SPI_CPHA_2Edge;                         //数据捕获于第二个时钟沿x
	spiTyp.SPI_NSS = SPI_NSS_Soft;                            //NSS信号由硬件（NSS管脚）还是软件（SSI位）管理
	spiTyp.SPI_FirstBit = SPI_FirstBit_MSB;                   //指定数据传输从MSB位开始
	spiTyp.SPI_CRCPolynomial = 7;                             //CRC值计算的多项式
	//如果速度设置输入0，则低速模式，非0则高速模式
	if(SpeedSet == SPI_SPEED_LOW)
		spiTyp.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //波特率预分频值为256
	else
		spiTyp.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;

	SPI_Init(SPI1, &spiTyp);                                  //初始化SPI1
}

/**
 * 等待SD卡Ready
 */
u8 sdWaitReady(void)
{
	uint32_t t = 0;
	while(t < 0xfffe)
	{
		if(spiReadWrite(DUMMY_BYTE) == DUMMY_BYTE)
			return 1;
		t ++;
	}
	return 0;
}

/**
 * 向SD卡发送一个命令
 * cmd  命令
 * arg  命令参数
 * crc  crc校验值
 */
u8 sdSendCommand(u8 cmd, u32 arg, u8 crc)
{
	unsigned char r1;
	unsigned char retry = 0;

	//Dummy byte and chip enable
	spiReadWrite(DUMMY_BYTE);
	//片选端置低，选中SD卡
	SD_CS_ENABLE();

	//Command, argument and crc
	spiReadWrite(cmd | 0x40);
	spiReadWrite(arg >> 24);
	spiReadWrite(arg >> 16);
	spiReadWrite(arg >> 8);
	spiReadWrite(arg);
	spiReadWrite(crc);

	//Wait response, quit till timeout
	for(retry = 0; retry < 200; retry++)
	{
		r1 = spiReadWrite(DUMMY_BYTE);
		if(r1 != DUMMY_BYTE)
			break;
	}

	//关闭片选
	SD_CS_DISABLE();
	//在总线上额外增加8个时钟，让SD卡完成剩下的工作
	spiReadWrite(DUMMY_BYTE);

	//返回状态值
	return r1;
}


u8 sdSendCommandNoDeassert(u8 cmd, u32 arg, u8 crc)
{
	unsigned char r1;
	unsigned char retry = 0;

	//Dummy byte and chip enable
	spiReadWrite(DUMMY_BYTE);
	//片选端置低，选中SD卡
	SD_CS_ENABLE();

	//Command, argument and crc
	spiReadWrite(cmd | 0x40);
	spiReadWrite(arg >> 24);
	spiReadWrite(arg >> 16);
	spiReadWrite(arg >> 8);
	spiReadWrite(arg);
	spiReadWrite(crc);

	//Wait response, quit till timeout
	for(retry = 0; retry < 200; retry++)
	{
		r1 = spiReadWrite(DUMMY_BYTE);
		if(r1 != DUMMY_BYTE)
			break;
	}
	//返回响应值
	return r1;
}
/**
 * 初始化SD卡
 */
u8 sdInit(void)
{
	u16 i;      // 用来循环计数
	u8 r1;      // 存放SD卡的返回值
	u16 retry;  // 用来进行超时计数
	u8 buff[6];

	//如果没有检测到卡插入，直接退出，返回错误标志
	if(!sdDetect())
		return SD_NO_CARD;

	sdPowerOn();

	//Satrt send 74 clocks at least
	for(i = 0; i < 210; i++)
		spiReadWrite(DUMMY_BYTE);

	//Start send CMD0 till return 0x01 means in IDLE state
	for(i = 0; i < 210; i++)
	{
		r1 = sdSendCommand(CMD0, 0, 0x95);
		if(r1 == 0x01)
		{
			i = 0;
			break;
		}
	}

	//Timeout return
	if(i != 0)
		return SD_TIMEOUT;

	//Get the card type, version
	r1 = sdSendCommandNoDeassert(CMD8, 0x1AA, 0x87);

	if(r1 == 0x05)
	{
		//r1=0x05 -> V1.0
		//设置卡类型为SDV1.0，如果后面检测到为MMC卡，再修改为MMC
		cardInfo.CardType = CARDTYPE_SDV1;

		//如果是V1.0卡，CMD8指令后没有后续数据
		//片选置高，结束本次命令
		SD_CS_DISABLE();
		//多发8个CLK，让SD结束后续操作
		spiReadWrite(DUMMY_BYTE);

		//Send CMD55+ACMD41, No-response is a MMC card, otherwise is a SD1.0 card
		for(retry = 0; retry < 0x10; retry++)
		{
			//先发CMD55，应返回0x01；否则出错
			r1 = sdSendCommand(CMD55, 0, 0);
			if(r1 != 0x01)
				//return r1;
				continue;
			//得到正确响应后，发ACMD41，应得到返回值0x00，否则重试200次
			r1 = sdSendCommand(ACMD41, 0, 0);
			if(r1 == 0x00)
			{
				retry = 0;
				break;
			}
		}

		//MMC card initialize
		if(retry != 0)
		{
			for(retry = 0; retry < 0xFFF; retry++)
			{
				r1 = sdSendCommand(CMD1, 0, 0);		//should be return 0x00
				if(r1 == 0x00)
				{
					retry = 0;
					break;
				}
			}
			//Timeout return
			if(retry != 0)
				return SD_TIMEOUT;

			cardInfo.CardType = CARDTYPE_MMC;
		}

		//设置SPI为高速模式
		spiSetSpeed(SPI_SPEED_HIGH);
		spiReadWrite(DUMMY_BYTE);

		//CRC disable
		r1 = sdSendCommand(CMD59, 0, 0x01);
		if(r1 != 0x00)
			return r1;  //response error, return r1

		//Set the block size
		r1 = sdSendCommand(CMD16, MSD_BLOCKSIZE, 0xFF);
		if(r1 != 0x00)
			return r1;  //response error, return r1

	}
	else if(r1 == 0x01)
	{
		//r1=0x01 -> V2.x, read OCR register, check version
		//4Bytes returned after CMD8 sent
		//V2.0的卡，CMD8命令后会传回4字节的数据，要跳过再结束本命令
		buff[0] = spiReadWrite(DUMMY_BYTE);  //should be 0x00
		buff[1] = spiReadWrite(DUMMY_BYTE);  //should be 0x00
		buff[2] = spiReadWrite(DUMMY_BYTE);  //should be 0x01
		buff[3] = spiReadWrite(DUMMY_BYTE);  //should be 0xAA

		//End of CMD8, chip disable and dummy byte
		SD_CS_DISABLE();
		//the next 8 clocks
		spiReadWrite(DUMMY_BYTE);

		//Check voltage range be 2.7-3.6V
		if(buff[2]==0x01 && buff[3]==0xAA)
		{
			for(retry = 0; retry < 0x1000; retry++)
			{
				r1 = sdSendCommand(CMD55, 0, 0);			// should be return 0x01
				if(r1 != 0x01)
					//return r1;
					continue;

				r1 = sdSendCommand(ACMD41, 0x40000000, 0);	// should be return 0x00
				if(r1 == 0x00)
				{
					retry = 0;
					break;
				}
			}

			// Timeout return
			if(retry != 0)
				return SD_TIMEOUT;

			//Read OCR by CMD58
			r1 = sdSendCommandNoDeassert(CMD58, 0, 0);
			if(r1 != 0x00)
				return r1;  //response error, return r1

			//读OCR指令发出后，紧接着是4字节的OCR信息
			buff[0] = spiReadWrite(DUMMY_BYTE);
			buff[1] = spiReadWrite(DUMMY_BYTE);
			buff[2] = spiReadWrite(DUMMY_BYTE);
			buff[3] = spiReadWrite(DUMMY_BYTE);

			//End of CMD58, chip disable and dummy byte
			SD_CS_DISABLE();
			spiReadWrite(DUMMY_BYTE);

			//OCR -> CCS(bit30)  1: SDV2HC	 0: SDV2
			if(buff[0] & 0x40)    //检查CCS
				cardInfo.CardType = CARDTYPE_SDV2HC;
			else
				cardInfo.CardType = CARDTYPE_SDV2;

			//设置SPI为高速模式
			spiSetSpeed(SPI_SPEED_HIGH);
			spiReadWrite(DUMMY_BYTE);
		}
	}
	return 0;
}

/**
 * 从SD卡中读回指定长度的数据，放置在给定位置
 * *data(存放读回数据的内存>len)
 * len(数据长度）
 * release(传输完成后是否释放总线CS置高 0：不释放 1：释放）
 */
u8 sdReceiveData(u8 *data, u16 len, u8 release)
{
	u16 retry;
	u8 r1;

	//Card enable, Prepare to read
	SD_CS_ENABLE();
	for(retry = 0; retry < 2000; retry++)
	{
		r1 = spiReadWrite(DUMMY_BYTE);
		if(r1 == 0xFE)
		{
			retry = 0;
			break;
		}
	}
	/* Timeout return	*/
	if(retry != 0)
	{
		SD_CS_DISABLE();
		return SD_TIMEOUT;
	}

	//Start reading
	while(len--)
	{
		*data = spiReadWrite(DUMMY_BYTE);
		data++;
	}

	// 2bytes dummy CRC
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);

	//chip disable and dummy byte
	if(release)
	{
		SD_CS_DISABLE();
		spiReadWrite(DUMMY_BYTE);
	}

	return 0;
}

/**
 *  获取SD卡信息
 */
int sdGetCardInfo(PMSD_CARDINFO cardInfo)
{
	uint8_t r1;
	uint8_t CSD_Tab[16];
	uint8_t CID_Tab[16];

	//send CMD9, Read CSD
	r1 = sdSendCommand(CMD9, 0, 0xFF);
	if(r1 != 0x00)
		return r1;
	if(sdReceiveData(CSD_Tab, 16, 1))
		return SD_RECEIVE_ERROR;

	//Send CMD10, Read CID
	r1 = sdSendCommand(CMD10, 0, 0xFF);
	if(r1 != 0x00)
		return r1;
	if(sdReceiveData(CID_Tab, 16, 1))
		return SD_RECEIVE_ERROR;

	//Byte 0
	cardInfo->CSD.CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
	cardInfo->CSD.SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
	cardInfo->CSD.Reserved1 = CSD_Tab[0] & 0x03;
	//Byte 1
	cardInfo->CSD.TAAC = CSD_Tab[1] ;
	//Byte 2
	cardInfo->CSD.NSAC = CSD_Tab[2];
	//Byte 3
	cardInfo->CSD.MaxBusClkFrec = CSD_Tab[3];
	//Byte 4
	cardInfo->CSD.CardComdClasses = CSD_Tab[4] << 4;
	//Byte 5
	cardInfo->CSD.CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
	cardInfo->CSD.RdBlockLen = CSD_Tab[5] & 0x0F;
	//Byte 6
	cardInfo->CSD.PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
	cardInfo->CSD.WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
	cardInfo->CSD.RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
	cardInfo->CSD.DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
	cardInfo->CSD.Reserved2 = 0; /* Reserved */
	cardInfo->CSD.DeviceSize = (CSD_Tab[6] & 0x03) << 10;
	//Byte 7
	cardInfo->CSD.DeviceSize |= (CSD_Tab[7]) << 2;
	//Byte 8
	cardInfo->CSD.DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
	cardInfo->CSD.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
	cardInfo->CSD.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
	//Byte 9
	cardInfo->CSD.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
	cardInfo->CSD.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
	cardInfo->CSD.DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
	//Byte 10
	cardInfo->CSD.DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
	cardInfo->CSD.EraseGrSize = (CSD_Tab[10] & 0x7C) >> 2;
	cardInfo->CSD.EraseGrMul = (CSD_Tab[10] & 0x03) << 3;
	//Byte 11
	cardInfo->CSD.EraseGrMul |= (CSD_Tab[11] & 0xE0) >> 5;
	cardInfo->CSD.WrProtectGrSize = (CSD_Tab[11] & 0x1F);
	//Byte 12
	cardInfo->CSD.WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
	cardInfo->CSD.ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
	cardInfo->CSD.WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
	cardInfo->CSD.MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
	//Byte 13
	cardInfo->CSD.MaxWrBlockLen |= (CSD_Tab[13] & 0xc0) >> 6;
	cardInfo->CSD.WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
	cardInfo->CSD.Reserved3 = 0;
	cardInfo->CSD.ContentProtectAppli = (CSD_Tab[13] & 0x01);
	//Byte 14
	cardInfo->CSD.FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
	cardInfo->CSD.CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
	cardInfo->CSD.PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
	cardInfo->CSD.TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
	cardInfo->CSD.FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
	cardInfo->CSD.ECC = (CSD_Tab[14] & 0x03);
	//Byte 15
	cardInfo->CSD.CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
	cardInfo->CSD.Reserved4 = 1;

	if(cardInfo->CardType == CARDTYPE_SDV2HC)
	{
		//Byte 7
		cardInfo->CSD.DeviceSize = (u16)(CSD_Tab[8]) *256;
		//Byte 8
		cardInfo->CSD.DeviceSize += CSD_Tab[9] ;
	}

	cardInfo->Capacity = cardInfo->CSD.DeviceSize * MSD_BLOCKSIZE * 1024;
	cardInfo->BlockSize = MSD_BLOCKSIZE;

	//Byte 0
	cardInfo->CID.ManufacturerID = CID_Tab[0];
	//Byte 1
	cardInfo->CID.OEM_AppliID = CID_Tab[1] << 8;
	//Byte 2
	cardInfo->CID.OEM_AppliID |= CID_Tab[2];
	//Byte 3
	cardInfo->CID.ProdName1 = CID_Tab[3] << 24;
	//Byte 4
	cardInfo->CID.ProdName1 |= CID_Tab[4] << 16;
	//Byte 5
	cardInfo->CID.ProdName1 |= CID_Tab[5] << 8;
	//Byte 6
	cardInfo->CID.ProdName1 |= CID_Tab[6];
	//Byte 7
	cardInfo->CID.ProdName2 = CID_Tab[7];
	//Byte 8
	cardInfo->CID.ProdRev = CID_Tab[8];
	//Byte 9
	cardInfo->CID.ProdSN = CID_Tab[9] << 24;
	//Byte 10
	cardInfo->CID.ProdSN |= CID_Tab[10] << 16;
	//Byte 11
	cardInfo->CID.ProdSN |= CID_Tab[11] << 8;
	//Byte 12
	cardInfo->CID.ProdSN |= CID_Tab[12];
	//Byte 13
	cardInfo->CID.Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
	//Byte 14
	cardInfo->CID.ManufactDate = (CID_Tab[13] & 0x0F) << 8;
	//Byte 15
	cardInfo->CID.ManufactDate |= CID_Tab[14];
	//Byte 16
	cardInfo->CID.CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
	cardInfo->CID.Reserved2 = 1;

	return 0;
}

/**
 * 读SD卡的一个block
 */
u8 sdReadSingleBlock(u32 sector, u8 *buffer)
{
	u8 r1;

	//if ver = SD2.0 HC, sector need <<9
	if(cardInfo.CardType != CARDTYPE_SDV2HC)
		sector = sector<<9;

	//Send CMD17 : Read single block command
	r1 = sdSendCommand(CMD17, sector, 0);

	if(r1 != 0x00)
		return r1;

	r1 = sdReceiveData(buffer, MSD_BLOCKSIZE, 1);

	//Send stop data transmit command - CMD12
	sdSendCommand(CMD12, 0, 0);

	return r1;
}

/**
 * 写入SD卡的一个block
 */
u8 SD_WriteSingleBlock(u32 sector, const u8 *data)
{
	u8 r1;
	register u16 i;
	u16 retry;

	//if ver = SD2.0 HC, sector need <<9
	if(cardInfo.CardType != SD_TYPE_V2HC)
		sector = sector<<9;

	//Send CMD24 : Write single block command
	r1 = sdSendCommand(CMD24, sector, 0x00);
	if(r1 != 0x00)
		return r1;

	//Card enable, Prepare to write
	SD_CS_ENABLE();

	//先放3个空数据，等待SD卡准备好
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);

	// Start data write token: 0xFE
	spiReadWrite(0xFE);

	//Start single block write the data buffer
	for(i = 0; i < MSD_BLOCKSIZE; i++)
		spiReadWrite(*data++);

	//2Bytes dummy CRC
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);

	//MSD card accept the data
	r1 = spiReadWrite(DUMMY_BYTE);
	if((r1 & 0x1F) != 0x05)
	{
		SD_CS_DISABLE();
		return r1;
	}

	//Wait all the data programm finished
	retry = 0;
	while(!spiReadWrite(DUMMY_BYTE))
	{
		retry++;
		//Timeout return
		if(retry > 0xfffe)
		{
			SD_CS_DISABLE();
			return SD_TIMEOUT;
		}
	}

	//chip disable and dummy byte
	SD_CS_DISABLE();
	spiReadWrite(DUMMY_BYTE);

	return 0;
}

/**
 * 读SD卡的多个block
 */
u8 sdReadMultiBlock(u32 sector, u8 *buffer, u8 count)
{
	u8 r1;
	register uint32_t i;

	// if ver = SD2.0 HC, sector need <<9
	if(cardInfo.CardType != CARDTYPE_SDV2HC)
		sector = sector<<9;

	//Send CMD18 : Read multi block command
	r1 = sdSendCommand(CMD18, sector, 0);
	if(r1 != 0x00)
		return r1;

	//Start read
	for(i = 0; i < count; i++)
	{
		if(sdReceiveData(buffer + i*MSD_BLOCKSIZE, MSD_BLOCKSIZE, 0))
		{
			//Send stop data transmit command - CMD12
			sdSendCommand(CMD12, 0, 0);
			//chip disable and dummy byte
			SD_CS_DISABLE();
			spiReadWrite(DUMMY_BYTE);
			return SD_RECEIVE_ERROR;
		}
	}

	//Send stop data transmit command - CMD12
	sdSendCommand(CMD12, 0, 0);

	//chip disable and dummy byte
	SD_CS_DISABLE();
	spiReadWrite(DUMMY_BYTE);

	return 0;
}

/**
 * 写入SD卡的一个block
 */
u8 sdWriteSingleBlock(u32 sector, const u8 *data)
{
    u8 r1;
    register u16 i;
    u16 retry;

    //if ver = SD2.0 HC, sector need <<9
    if(cardInfo.CardType != SD_TYPE_V2HC)
        sector = sector<<9;

    //Send CMD24 : Write single block command
    r1 = sdSendCommand(CMD24, sector, 0x00);
    if(r1 != 0x00)
        return r1;

    //Card enable, Prepare to write
    SD_CS_ENABLE();
    //先放3个空数据，等待SD卡准备好
    spiReadWrite(DUMMY_BYTE);
    spiReadWrite(DUMMY_BYTE);
    spiReadWrite(DUMMY_BYTE);

    //Start data write token: 0xFE
    spiReadWrite(0xFE);

    //放Start single block write the data buffer
    for(i = 0; i < 512; i++)
    	spiReadWrite(*data++);

    //2Bytes dummy CRC
    spiReadWrite(DUMMY_BYTE);
    spiReadWrite(DUMMY_BYTE);

    //MSD card accept the data
    r1 = spiReadWrite(DUMMY_BYTE);
    if((r1 & 0x1F) != 0x05)
    {
        SD_CS_DISABLE();
        return r1;
    }

    //Wait all the data programm finished
    retry = 0;
    while(!spiReadWrite(DUMMY_BYTE))
    {
        retry++;
        // Timeout return
        if(retry > 0xfffe)
        {
            SD_CS_DISABLE();
            return SD_TIMEOUT;
        }
    }

    //chip disable and dummy byte
    SD_CS_DISABLE();
    spiReadWrite(DUMMY_BYTE);

    return 0;
}


/**
 * 写入SD卡的N个block
 */
u8 sdWriteMultiBlock(u32 sector, const u8 *data, u8 count)
{
	u8 r1;
	register uint16_t i;
	register uint16_t block;

	//if ver = SD2.0 HC, sector need <<9
	if(cardInfo.CardType != SD_TYPE_V2HC)
		sector = sector<<9;

	//Send command ACMD23 berfore multi write if is not a MMC card
	if(cardInfo.CardType != SD_TYPE_MMC)
		r1 = sdSendCommand(ACMD23, count, 0x00);

	//Send CMD25 : Write nulti block command
	r1 = sdSendCommand(CMD25, sector, 0x00);
	if(r1 != 0x00)
		return r1;

	//Card enable, Prepare to write
	SD_CS_ENABLE();

	//先放3个空数据，等待SD卡准备好
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);
	spiReadWrite(DUMMY_BYTE);

	for(block = 0; block < count; block++)
	{
		//Start multi block write token: 0xFC
		spiReadWrite(0xFC);
		for(i = 0; i < MSD_BLOCKSIZE; i++)
			spiReadWrite(*data++);

		//2Bytes dummy CRC
		spiReadWrite(DUMMY_BYTE);
		spiReadWrite(DUMMY_BYTE);

		//MSD card accept the data
		r1 = spiReadWrite(DUMMY_BYTE);
		if((r1 & 0x1F) != 0x05)
		{
			SD_CS_DISABLE();
			return r1;
		}

		//Wait all the data programm finished
		if(!sdWaitReady())
		{
			SD_CS_DISABLE();
			return SD_TIMEOUT;
		}
	}

	//Send end of transmit token: 0xFD
	r1 = spiReadWrite(0xFD);
	if(r1 == 0x00)
		return SD_RECEIVE_ERROR;

	if(!sdWaitReady())
	{
		SD_CS_DISABLE();
		return SD_TIMEOUT;
	}

	//写入完成，片选置1
	SD_CS_DISABLE();
	spiReadWrite(DUMMY_BYTE);

	return 0;
}

