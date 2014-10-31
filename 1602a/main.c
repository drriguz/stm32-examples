#include "stm32f10x.h"

/*******************************************************************************
* Function Name   : SD_MMC_SPI_Init
* Description        : SD_MMC_SPI_Init
* Input                : None
* Output             : None
* Return             : zero init success, non-zero init error
*******************************************************************************/
u8 SD_MMC_SPI_Init(void)
{
   GPIO_InitTypeDef GPIO_InitStructure;
   /* Enable SPI1 and GPIO clocks */
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA |
                     RCC_APB2Periph_SD_MMC_SPI_CS, ENABLE);
   /* Configure SPI1 pins: SCK, MISO and MOSI */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
   GPIO_Init(GPIOA, &GPIO_InitStructure);
   /* Configure SD_MMC_SPI_CS */
   GPIO_InitStructure.GPIO_Pin = SD_MMC_SPI_CS_Pin_CS;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
   GPIO_Init(SD_MMC_SPI_CS, &GPIO_InitStructure);

   ////////////////////////////////////////////////////////////////////////////////////////////////
/* initialize SPI with lowest frequency */
SD_MMC_Low_Speed();

/* card needs 74 cycles minimum to start up */
for(u8 i = 0; i < 10; ++i)
{
       /* wait 8 clock cycles */
       SD_MMC_ReadWrite_Byte(0x00);
}
/* address card */
SD_MMC_SPI_SELECT();
/* reset card */
u8 response;
for(u16 i = 0; ; ++i)
{
       response = SD_MMC_Send_Command(CMD_GO_IDLE_STATE , 0 );
       if( response == 0x01 )
         break;
       if(i == 0x1ff)
       {
         SD_MMC_SPI_DESELECT();
         return 1;
       }
}

/* wait for card to get ready */
for(u16 i = 0; ; ++i)
{
       response = SD_MMC_Send_Command(CMD_SEND_OP_COND, 0);
       if(!(response & (1 << R1_IDLE_STATE)))
         break;
       if(i == 0x7fff)
       {
         SD_MMC_SPI_DESELECT();
         return 1;
       }
}
/* set block size to 512 bytes */
if(SD_MMC_Send_Command(CMD_SET_BLOCKLEN, 512))
{
       SD_MMC_SPI_DESELECT();
       return 1;
}
/* deaddress card */
SD_MMC_SPI_DESELECT();
/* switch to highest SPI frequency possible */
SD_MMC_High_Speed();

return 0;
}

//发送一个命令
/*******************************************************************************
* Function Name   : SD_MMC_Send_Command
* Description        : SD_MMC_Send_Command
* Input                : None
* Output             : None
* Return             : None
*******************************************************************************/
u8 SD_MMC_Send_Command(u8 cmd, u32 arg)
{
       u8 Response;
       u8 Retry = 0;
       SD_MMC_ReadWrite_Byte(0xff);
       SD_MMC_SPI_SELECT();
       //分别写入命令
       SD_MMC_ReadWrite_Byte(cmd | 0x40);
       SD_MMC_ReadWrite_Byte(arg >> 24);
       SD_MMC_ReadWrite_Byte(arg >> 16);
       SD_MMC_ReadWrite_Byte(arg >> 8);
       SD_MMC_ReadWrite_Byte(arg);
       SD_MMC_ReadWrite_Byte(0x95);

       do{
            // 等待响应,响应的开始位为0
            Response = SD_MMC_ReadWrite_Byte(0xff);
            Retry++;
       }while( ((Response&0x80)!=0) && (Retry < 200) );
       SD_MMC_SPI_DESELECT();
       return Response;   //返回状态值
}

//读一个block块 读取成功返回0 非0 则读取失败
/*******************************************************************************
* Function Name   : SD_MMC_Read_Single_Block
* Description        : SD_MMC_Read_Single_Block
* Input                : sector number and buffer data point
* Output             : None
* Return             : zero success, non-zero error
*******************************************************************************/
u8 SD_MMC_Read_Single_Block(u32 sector, u8* buffer)
{
       u8 Response;
       u16 i;
       u16 Retry = 0;
      //读命令 send read command
       Response = SD_MMC_Send_Command(CMD_READ_SINGLE_BLOCK, sector<<9);
       if(Response != 0x00)
            return Response;
       SD_MMC_SPI_SELECT();
       // start byte 0xfe
       while(SD_MMC_ReadWrite_Byte(0xff) != 0xfe)
       {
      if(++Retry > 0xfffe)
      {
             SD_MMC_SPI_DESELECT();
             return 1; //timeout
      }
       }
       for(i = 0; i < 512; ++i)
       {
            //读512个数据
            *buffer++ = SD_MMC_ReadWrite_Byte(0xff);
       }
       SD_MMC_ReadWrite_Byte(0xff);   //伪crc
       SD_MMC_ReadWrite_Byte(0xff);   //伪crc
       SD_MMC_SPI_DESELECT();
       SD_MMC_ReadWrite_Byte(0xff);   // extra 8 CLK
       return 0;
}


//写一个block块 成功返回0 非0 则写入失败
/*******************************************************************************
* Function Name   : SD_MMC_Write_Single_Block
* Description        : SD_MMC_Write_Single_Block
* Input                : sector number and buffer data point
* Output             : None
* Return             : zero success, non-zero error.
*******************************************************************************/
u8 SD_MMC_Write_Single_Block(u32 sector, u8* buffer)
{
u8 Response;
u16 i;
u16 retry=0;

       //写命令   send write command
Response = SD_MMC_Send_Command(CMD_WRITE_SINGLE_BLOCK, sector<<9);
if(Response != 0x00)
return Response;
SD_MMC_SPI_SELECT();

SD_MMC_ReadWrite_Byte(0xff);
SD_MMC_ReadWrite_Byte(0xff);
SD_MMC_ReadWrite_Byte(0xff);
       //发开始符 start byte 0xfe
SD_MMC_ReadWrite_Byte(0xfe);

       //送512字节数据 send 512 bytes data
for(i=0; i<512; i++)
{
   SD_MMC_ReadWrite_Byte(*buffer++);
}

SD_MMC_ReadWrite_Byte(0xff); //dummy crc
SD_MMC_ReadWrite_Byte(0xff); //dummy crc

Response = SD_MMC_ReadWrite_Byte(0xff);

//等待是否成功 judge if it successful
if( (Response&0x1f) != 0x05)
{
   SD_MMC_SPI_DESELECT();
   return Response;
}
//等待操作完   wait no busy
while(SD_MMC_ReadWrite_Byte(0xff) != 0x00)
{
   if(retry++ > 0xfffe)
   {
SD_MMC_SPI_DESELECT();
return 1;
   }
}
SD_MMC_SPI_DESELECT();
SD_MMC_ReadWrite_Byte(0xff);// extra 8 CLK
return 0;
}
