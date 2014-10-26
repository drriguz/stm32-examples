#include "stm32f10x_conf.h"
#include "sdcard.h"

MSD_CARDINFO cardInfo;


/**
 * ���SD���Ƿ����
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
	//NSS ��ʼ��Ϊ�������
	gpioTyp.GPIO_Pin = GPIO_Pin_4;
	gpioTyp.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpioTyp);

	//ֹͣSPI
	SPI_Cmd(SPI1, DISABLE);

	spiSetSpeed(SPI_SPEED_LOW);
	//ʹ��SPI����
	SPI_Cmd(SPI1, ENABLE);
}

/**
 * SPI��дһ���ֽڣ�������ɺ󷵻ر���ͨѶ��ȡ�����ݣ�
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
 * SPI��ʼ����ָ�����ٻ����Ǹ���ģʽ
 * SPI_SPEED_HIGH   1
 * SPI_SPEED_LOW    0
 */
void spiSetSpeed(u8 SpeedSet)
{
	SPI_InitTypeDef  spiTyp;
	//SPI1 ����
	spiTyp.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   //SPIΪ˫��˫��ȫ˫��
	spiTyp.SPI_Mode = SPI_Mode_Master;                        //SPI����ģʽ����SPI
	spiTyp.SPI_DataSize = SPI_DataSize_8b;                    //SPI���ݴ�С��SPI���ͽ���8λ֡�ṹ
	spiTyp.SPI_CPOL = SPI_CPOL_High;                          //ѡ����ʱ�ӵ���̬��ʱ�����ո�x
	spiTyp.SPI_CPHA = SPI_CPHA_2Edge;                         //���ݲ����ڵڶ���ʱ����x
	spiTyp.SPI_NSS = SPI_NSS_Soft;                            //NSS�ź���Ӳ����NSS�ܽţ����������SSIλ������
	spiTyp.SPI_FirstBit = SPI_FirstBit_MSB;                   //ָ�����ݴ����MSBλ��ʼ
	spiTyp.SPI_CRCPolynomial = 7;                             //CRCֵ����Ķ���ʽ
	//����ٶ���������0�������ģʽ����0�����ģʽ
	if(SpeedSet == SPI_SPEED_LOW)
		spiTyp.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; //������Ԥ��ƵֵΪ256
	else
		spiTyp.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;

	SPI_Init(SPI1, &spiTyp);                                  //��ʼ��SPI1
}

/**
 * �ȴ�SD��Ready
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
 * ��SD������һ������
 * cmd  ����
 * arg  �������
 * crc  crcУ��ֵ
 */
u8 sdSendCommand(u8 cmd, u32 arg, u8 crc)
{
	unsigned char r1;
	unsigned char retry = 0;

	//Dummy byte and chip enable
	spiReadWrite(DUMMY_BYTE);
	//Ƭѡ���õͣ�ѡ��SD��
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

	//�ر�Ƭѡ
	SD_CS_DISABLE();
	//�������϶�������8��ʱ�ӣ���SD�����ʣ�µĹ���
	spiReadWrite(DUMMY_BYTE);

	//����״ֵ̬
	return r1;
}


u8 sdSendCommandNoDeassert(u8 cmd, u32 arg, u8 crc)
{
	unsigned char r1;
	unsigned char retry = 0;

	//Dummy byte and chip enable
	spiReadWrite(DUMMY_BYTE);
	//Ƭѡ���õͣ�ѡ��SD��
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
	//������Ӧֵ
	return r1;
}
/**
 * ��ʼ��SD��
 */
u8 sdInit(void)
{
	u16 i;      // ����ѭ������
	u8 r1;      // ���SD���ķ���ֵ
	u16 retry;  // �������г�ʱ����
	u8 buff[6];

	//���û�м�⵽�����룬ֱ���˳������ش����־
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
		//���ÿ�����ΪSDV1.0����������⵽ΪMMC�������޸�ΪMMC
		cardInfo.CardType = CARDTYPE_SDV1;

		//�����V1.0����CMD8ָ���û�к�������
		//Ƭѡ�øߣ�������������
		SD_CS_DISABLE();
		//�෢8��CLK����SD������������
		spiReadWrite(DUMMY_BYTE);

		//Send CMD55+ACMD41, No-response is a MMC card, otherwise is a SD1.0 card
		for(retry = 0; retry < 0x10; retry++)
		{
			//�ȷ�CMD55��Ӧ����0x01���������
			r1 = sdSendCommand(CMD55, 0, 0);
			if(r1 != 0x01)
				//return r1;
				continue;
			//�õ���ȷ��Ӧ�󣬷�ACMD41��Ӧ�õ�����ֵ0x00����������200��
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

		//����SPIΪ����ģʽ
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
		//V2.0�Ŀ���CMD8�����ᴫ��4�ֽڵ����ݣ�Ҫ�����ٽ���������
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

			//��OCRָ����󣬽�������4�ֽڵ�OCR��Ϣ
			buff[0] = spiReadWrite(DUMMY_BYTE);
			buff[1] = spiReadWrite(DUMMY_BYTE);
			buff[2] = spiReadWrite(DUMMY_BYTE);
			buff[3] = spiReadWrite(DUMMY_BYTE);

			//End of CMD58, chip disable and dummy byte
			SD_CS_DISABLE();
			spiReadWrite(DUMMY_BYTE);

			//OCR -> CCS(bit30)  1: SDV2HC	 0: SDV2
			if(buff[0] & 0x40)    //���CCS
				cardInfo.CardType = CARDTYPE_SDV2HC;
			else
				cardInfo.CardType = CARDTYPE_SDV2;

			//����SPIΪ����ģʽ
			spiSetSpeed(SPI_SPEED_HIGH);
			spiReadWrite(DUMMY_BYTE);
		}
	}
	return 0;
}

/**
 * ��SD���ж���ָ�����ȵ����ݣ������ڸ���λ��
 * *data(��Ŷ������ݵ��ڴ�>len)
 * len(���ݳ��ȣ�
 * release(������ɺ��Ƿ��ͷ�����CS�ø� 0�����ͷ� 1���ͷţ�
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
 *  ��ȡSD����Ϣ
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
 * ��SD����һ��block
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
 * д��SD����һ��block
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

	//�ȷ�3�������ݣ��ȴ�SD��׼����
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
 * ��SD���Ķ��block
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
 * д��SD����һ��block
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
    //�ȷ�3�������ݣ��ȴ�SD��׼����
    spiReadWrite(DUMMY_BYTE);
    spiReadWrite(DUMMY_BYTE);
    spiReadWrite(DUMMY_BYTE);

    //Start data write token: 0xFE
    spiReadWrite(0xFE);

    //��Start single block write the data buffer
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
 * д��SD����N��block
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

	//�ȷ�3�������ݣ��ȴ�SD��׼����
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

	//д����ɣ�Ƭѡ��1
	SD_CS_DISABLE();
	spiReadWrite(DUMMY_BYTE);

	return 0;
}

