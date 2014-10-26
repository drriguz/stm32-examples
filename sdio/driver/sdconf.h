#ifndef _SOLEE_SDIO_CONFIG_H_
#define _SOLEE_SDIO_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* Connection:
 *  ___________________
 * /  1 2 3 4 5 6 7 8 |
 * |9                 |
 *
 * 1 DAT3    I/O/PP  -> PC.11
 * 2 CMD     PP      -> PD.12
 * 3 VSS1    S       -> GND
 * 4 Vdd     S       -> +3.3v
 * 5 CLK     I       -> PC.12
 * 6 Vss2    S       -> GND
 * 7 DAT0    I/O/PP  -> PC.08
 * 8 DAT1    I/O/PP  -> PC.09
 * 9 DAT2    I/O/PP  -> PC.10
 *
 * Detect            -> PF.11
 * WriteProtect      ->
 */


/**
 * @brief  SD FLASH SDIO Interface
 */

#define SD_DETECT_PIN                    GPIO_Pin_11                 /* PF.11 */
#define SD_DETECT_GPIO_PORT              GPIOF                       /* GPIOF */
#define SD_DETECT_GPIO_CLK               RCC_APB2Periph_GPIOF

#define SDIO_FIFO_ADDRESS                ((uint32_t)0x40018080)
/**
 * @brief  SDIO Intialization Frequency (400KHz max)
 */
#define SDIO_INIT_CLK_DIV                ((uint8_t)0xB2)
/**
 * @brief  SDIO Data Transfer Frequency (25MHz max)
 */
#define SDIO_TRANSFER_CLK_DIV            ((uint8_t)0x00)

void SD_LowLevel_DeInit(void);
void SD_LowLevel_Init(void);
void SD_LowLevel_DMA_TxConfig(uint32_t *BufferSRC, uint32_t BufferSize);
void SD_LowLevel_DMA_RxConfig(uint32_t *BufferDST, uint32_t BufferSize);
uint32_t SD_DMAEndOfTransferStatus(void);

#ifdef __cplusplus
}
#endif

#endif
