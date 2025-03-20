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
#pragma once

#include "EmwIoInterface.hpp"
#include "EmwOsInterface.hpp"
#include "stm32u5xx_hal.h"
#include <cstdint>

class EmwIoSpi final : public EmwIoInterface<EmwIoSpi> {
  public:
    EmwIoSpi(void) noexcept;
  public:
    ~EmwIoSpi(void) noexcept override;
  public:
    void initializeImp(EmwIoInterfaceTypes::InitializationMode mode) noexcept;
  public:
    void pollDataImp(std::uint32_t timeoutInMs) noexcept;
  public:
    void processPollingDataImp(std::uint32_t timeoutInMs) noexcept;
  public:
    std::uint16_t sendImp(const std::uint8_t *dataPtr, std::uint16_t dataLength) noexcept;
  public:
    std::int8_t unInitializeImp(void) noexcept;

  public:
    struct Stm32Hw_s {
      Stm32Hw_s(void) noexcept
        : hSpiPtr(nullptr), hNssPortPtr(nullptr), nssPin(0U)
        , hExtiFlowPtr(nullptr), hFlowPortPtr(nullptr), flowPin(0U), flowIrq(static_cast<IRQn_Type>(0))
        , hExtiNotifyPtr(nullptr), hNotifyPortPtr(nullptr), notifyPin(0U), notifyIrq(static_cast<IRQn_Type>(0))
        , hResetPortPtr(nullptr), resetPin(0U) {}
      SPI_HandleTypeDef *hSpiPtr;
      GPIO_TypeDef *hNssPortPtr;
      std::uint16_t nssPin;
      EXTI_HandleTypeDef *hExtiFlowPtr;
      GPIO_TypeDef *hFlowPortPtr;
      std::uint16_t flowPin;
      IRQn_Type flowIrq;
      EXTI_HandleTypeDef *hExtiNotifyPtr;
      GPIO_TypeDef *hNotifyPortPtr;
      std::uint16_t notifyPin;
      IRQn_Type notifyIrq;
      GPIO_TypeDef *hResetPortPtr;
      std::uint16_t resetPin;
    };

  private:
    std::int32_t exchangeHeaders(std::uint16_t txLength, std::uint16_t &rxLength) noexcept;
  private:
    bool isNotifyHigh(void) noexcept;
  private:
    bool isFlowLow(void) noexcept;
  private:
    void resetHardware(void) const noexcept;
  private:
    HAL_StatusTypeDef receive(std::uint8_t *rxDataPtr, std::uint16_t rxDataLength) noexcept;
  private:
    void setChipSelectHigh(void) const noexcept;
  private:
    void setChipSelectLow(void) const noexcept;
  private:
    void start(void) noexcept;
  private:
    void stop(void) noexcept;
  private:
    HAL_StatusTypeDef transmit(const std::uint8_t *txDataPtr, std::uint16_t dataLength) noexcept;
  private:
    HAL_StatusTypeDef transmitReceive(const std::uint8_t *txDataPtr, std::uint8_t *rxDataPtr,
                                      std::uint16_t dataLength) noexcept;
  private:
    std::int8_t waitFlowHigh(void) noexcept;

#if defined(EMW_WITH_RTOS)
  private:
    static EmwOsInterface::Thread_t IoThread;
  private:
    static volatile bool IoThreadQuitFlag;
  private:
    static void IoThreadFunction(EmwOsInterface::ThreadFunctionArgument_t argumentPtr) noexcept;
#endif /* EMW_WITH_RTOS */

  private:
    static void FlowInterruptCallback(void) noexcept;
  private:
    static void NotifyInterruptCallback(void) noexcept;
  private:
    static void SpiTransferCallback(SPI_HandleTypeDef *spiPtr) noexcept;
  private:
    static void SpiErrorCallback(SPI_HandleTypeDef *spiPtr) noexcept;

  private:
    struct Stm32Hw_s configuration;
  private:
    static EmwOsInterface::Semaphore_t FowRiseSem;
  private:
    static EmwOsInterface::Semaphore_t TransferDoneSem;
  private:
    static const std::uint8_t *TxDataAddress;
  private:
    static std::uint16_t TxDataLength;
  private:
    static EmwOsInterface::Mutex_t TxLock;
  private:
    static EmwOsInterface::Semaphore_t TxRxSem;
  private:
    static const std::uint16_t SPI_MAX_BYTE_COUNT = 2500U;
  private:
    static const std::uint32_t TIMEOUT_HARDWARE_EMW_MS = 2000U;
};
