/**
  ******************************************************************************
  * Copyright (C) 2025 C.Fenard.
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program. If not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
  */
#include "spi.h"
#include "stm32u5xx_hal_cortex.h"
#include "stm32u5xx_hal_dma.h"
#include "stm32u5xx_hal_gpio.h"
#include "stm32u5xx_hal_rcc.h"
#include "main.hpp"

DMA_HandleTypeDef hGpdma1Channel5;
DMA_HandleTypeDef hGpdma1Channel4;
SPI_HandleTypeDef hSpi2;

void InitializeSPI2(void)
{
  hSpi2.Instance = SPI2;
  hSpi2.Init.Mode = SPI_MODE_MASTER;
  hSpi2.Init.Direction = SPI_DIRECTION_2LINES;
  hSpi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hSpi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hSpi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hSpi2.Init.NSS = SPI_NSS_SOFT;
  hSpi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hSpi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hSpi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hSpi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hSpi2.Init.CRCPolynomial = 0x7;
  hSpi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hSpi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hSpi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hSpi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hSpi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hSpi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hSpi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hSpi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hSpi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hSpi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;

  if (HAL_SPI_Init(&hSpi2) != HAL_OK) {
    ErrorHandler();
  }
  {
    SPI_AutonomousModeConfTypeDef autonomous_mode = {0};

    autonomous_mode.TriggerState = SPI_AUTO_MODE_DISABLE;
    autonomous_mode.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
    autonomous_mode.TriggerPolarity = SPI_TRIG_POLARITY_RISING;

    if (HAL_SPIEx_SetConfigAutonomousMode(&hSpi2, &autonomous_mode) != HAL_OK) {
      ErrorHandler();
    }
  }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *spiPtr)
{
  if (spiPtr->Instance == SPI2) {
    RCC_PeriphCLKInitTypeDef configuration = {0};
    configuration.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    configuration.Spi2ClockSelection = RCC_SPI2CLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&configuration) != HAL_OK) {
      ErrorHandler();
    }
  }
  __HAL_RCC_SPI2_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  {
    GPIO_InitTypeDef configuration = {0};
    configuration.Pin = GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_1;
    configuration.Mode = GPIO_MODE_AF_PP;
    configuration.Pull = GPIO_NOPULL;
    configuration.Speed = GPIO_SPEED_FREQ_HIGH;
    configuration.Alternate = GPIO_AF5_SPI2;

    HAL_GPIO_Init(GPIOD, &configuration);
  }
  hGpdma1Channel5.Instance = GPDMA1_Channel5;
  hGpdma1Channel5.Init.Request = GPDMA1_REQUEST_SPI2_TX;
  hGpdma1Channel5.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
  hGpdma1Channel5.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hGpdma1Channel5.Init.SrcInc = DMA_SINC_INCREMENTED;
  hGpdma1Channel5.Init.DestInc = DMA_DINC_FIXED;
  hGpdma1Channel5.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
  hGpdma1Channel5.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
  hGpdma1Channel5.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
  hGpdma1Channel5.Init.SrcBurstLength = 1;
  hGpdma1Channel5.Init.DestBurstLength = 1;
  hGpdma1Channel5.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
  hGpdma1Channel5.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
  hGpdma1Channel5.Init.Mode = DMA_NORMAL;

  if (HAL_DMA_Init(&hGpdma1Channel5) != HAL_OK) {
    ErrorHandler();
  }
  __HAL_LINKDMA(spiPtr, hdmatx, hGpdma1Channel5);

  if (HAL_DMA_ConfigChannelAttributes(&hGpdma1Channel5, DMA_CHANNEL_NPRIV) != HAL_OK) {
    ErrorHandler();
  }

  hGpdma1Channel4.Instance = GPDMA1_Channel4;
  hGpdma1Channel4.Init.Request = GPDMA1_REQUEST_SPI2_RX;
  hGpdma1Channel4.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
  hGpdma1Channel4.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hGpdma1Channel4.Init.SrcInc = DMA_SINC_FIXED;
  hGpdma1Channel4.Init.DestInc = DMA_DINC_INCREMENTED;
  hGpdma1Channel4.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
  hGpdma1Channel4.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
  hGpdma1Channel4.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;
  hGpdma1Channel4.Init.SrcBurstLength = 1;
  hGpdma1Channel4.Init.DestBurstLength = 1;
  hGpdma1Channel4.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
  hGpdma1Channel4.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
  hGpdma1Channel4.Init.Mode = DMA_NORMAL;

  if (HAL_DMA_Init(&hGpdma1Channel4) != HAL_OK) {
    ErrorHandler();
  }
  __HAL_LINKDMA(spiPtr, hdmarx, hGpdma1Channel4);

  if (HAL_DMA_ConfigChannelAttributes(&hGpdma1Channel4, DMA_CHANNEL_NPRIV) != HAL_OK) {
    ErrorHandler();
  }
  HAL_NVIC_SetPriority(SPI2_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiPtr)
{
  if (spiPtr->Instance == SPI2) {
    __HAL_RCC_SPI2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_1);
    HAL_DMA_DeInit(spiPtr->hdmatx);
    HAL_DMA_DeInit(spiPtr->hdmarx);
    HAL_NVIC_DisableIRQ(SPI2_IRQn);
  }
}
