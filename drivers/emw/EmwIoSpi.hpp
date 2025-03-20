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

#include "emw_conf.hpp"
#include "EmwIoInterface.hpp"
#include "EmwOsInterface.hpp"
#include <cstdint>

class EmwApiCore;

class EmwIoSpi final : public EmwIoInterface {
  public:
    EmwIoSpi(void) : configuration() {}
  public:
    ~EmwIoSpi(void) override;
  public:
    void initialize(const class EmwApiCore &core, EmwIoInterface::InitializationMode mode) override;
  public:
    void pollData(const class EmwApiCore &core, uint32_t timeoutInMs) override;
  public:
    void processPollingData(const class EmwApiCore &core, uint32_t timeoutInMs) override;
  public:
    uint16_t send(const class EmwApiCore &core, const uint8_t *dataPtr, uint16_t dataLength) override;
  public:
    int8_t unInitialize(const class EmwApiCore &core) override;

  public:
    struct Stm32Hw_s {
      Stm32Hw_s(void)
        : hSpiPtr(nullptr), hNssPortPtr(nullptr), nssPin(0U)
        , hExtiFlowPtr(nullptr), hFlowPortPtr(nullptr), flowPin(0U), flowIrq(static_cast<IRQn_Type>(0))
        , hExtiNotifyPtr(nullptr), hNotifyPortPtr(nullptr), notifyPin(0U), notifyIrq(static_cast<IRQn_Type>(0))
        , hResetPortPtr(nullptr), resetPin(0U) {}
      SPI_HandleTypeDef *hSpiPtr;
      GPIO_TypeDef *hNssPortPtr;
      uint16_t nssPin;
      EXTI_HandleTypeDef *hExtiFlowPtr;
      GPIO_TypeDef *hFlowPortPtr;
      uint16_t flowPin;
      IRQn_Type flowIrq;
      EXTI_HandleTypeDef *hExtiNotifyPtr;
      GPIO_TypeDef *hNotifyPortPtr;
      uint16_t notifyPin;
      IRQn_Type notifyIrq;
      GPIO_TypeDef *hResetPortPtr;
      uint16_t resetPin;
    };

  private:
    int32_t exchangeHeaders(uint16_t txLength, uint16_t *rxLengthPtr);
  private:
    bool isNotifyHigh(void);
  private:
    bool isFlowLow(void);
  private:
    void resetHardware(void) const;
  private:
    HAL_StatusTypeDef receive(uint8_t *rxDataPtr, uint16_t rxDataLength);
  private:
    void setChipSelectHigh(void) const;
  private:
    void setChipSelectLow(void) const;
  private:
    void start(const class EmwApiCore &core);
  private:
    int8_t stop(const class EmwApiCore &core);
  private:
    HAL_StatusTypeDef transmit(const uint8_t *txDataPtr, uint16_t dataLength);
  private:
    HAL_StatusTypeDef transmitReceive(const uint8_t *txDataPtr, uint8_t *rxDataPtr, uint16_t dataLength);
  private:
    int8_t waitFlowHigh(void);

#if defined(EMW_WITH_RTOS)
  private:
    static EmwOsInterface::thread_t IoThread;
  private:
    static volatile bool IoThreadQuitFlag;
  private:
    static void IoThreadFunction(EmwOsInterface::thread_function_argument argument);
#endif /* EMW_WITH_RTOS */

  private:
    static void flowInterruptCallback(void);
  private:
    static void notifyInterruptCallback(void);
  private:
    static void spiTransferCallback(SPI_HandleTypeDef *spiPtr);
  private:
    static void spiErrorCallback(SPI_HandleTypeDef *spiPtr);

  private:
    struct Stm32Hw_s configuration;
  private:
    static EmwOsInterface::semaphore_t fowRiseSem;
  private:
    static EmwOsInterface::semaphore_t transferDoneSem;
  private:
    static const uint8_t *txDataAddress;
  private:
    static uint16_t txDataLength;
  private:
    static EmwOsInterface::mutex_t txLock;
  private:
    static EmwOsInterface::semaphore_t txRxSem;
  private:
    static const uint16_t SPI_MAX_BYTE_COUNT = 2500U;
  private:
    static const uint32_t TIMEOUT_HARDWARE_EMW_MS = 2000U;
};
