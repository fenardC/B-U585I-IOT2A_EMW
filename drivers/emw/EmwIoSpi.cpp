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
#include "EmwIoSpi.hpp"
#include "EmwApiBase.hpp"
#include "EmwApiCore.hpp"
#include "EmwCoreHci.hpp"
#include "EmwIoHardware.hpp"
#include "EmwNetworkStack.hpp"
#include "EmwOsInterface.hpp"
#include "emw_conf.hpp"
#include <cinttypes>
#include <cstdbool>
#include <cstdint>
#include <cstring>

#if !defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)
#define DEBUG_IO_WARNING(...)
#else
#define DEBUG_DETAILS_IO_LOG
#endif /* EMW_IO_DEBUG */

typedef __PACKED_STRUCT {
  uint8_t type;
  uint16_t len;
  uint16_t lenx;
  uint8_t dummy[3];
} SpiHeader_t;

EmwIoSpi::~EmwIoSpi(void)
{
  DEBUG_IO_LOG("\nEmwIoSpi::~EmwIoSpi()<\n")
}

void EmwIoSpi::initialize(const class EmwApiCore &core, EmwIoInterface::InitializationMode mode)
{
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::initialize()>\n", HAL_GetTick())

  if (EmwIoInterface::eRESET == mode) {
    const int32_t status = EmwIoHardware::initialize(this->configuration);
    EmwOsInterface::assertAlways(0 == status);
    this->setChipSelectHigh();
    this->resetHardware();
  }
  else {
    this->start(core);
  }
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::initialize()<\n\n", HAL_GetTick())
}

void EmwIoSpi::pollData(const class EmwApiCore &core, uint32_t timeoutInMs)
{
#if defined(EMW_WITH_NO_OS)
  this->processPollingData(core, timeoutInMs);
#else
  (void) core;
  (void) timeoutInMs;
#endif /* EMW_WITH_NO_OS */
}

void EmwIoSpi::processPollingData(const class EmwApiCore &core, uint32_t timeoutInMs)
{
  static EmwNetworkStack::Buffer_t *network_buffer_ptr = nullptr;
  bool first_miss = true;

  (void)core;
  this->setChipSelectHigh();

  while (nullptr == network_buffer_ptr) {
    network_buffer_ptr = static_cast<EmwNetworkStack::Buffer_t *>(EmwNetworkStack::allocBuffer());
    if (nullptr == network_buffer_ptr) {
#if defined(EMW_WITH_RTOS)
      /* Be cooperative */
      (void) EmwOsInterface::delayTicks(1U);
#endif /* EMW_WITH_RTOS */
      if (first_miss) {
        first_miss = false;
        DEBUG_IO_WARNING("Running out of buffer for RX\n")
      }
    }
  }
  if (EmwOsInterface::eOK == EmwOsInterface::takeSemaphore(&EmwIoSpi::txRxSem, timeoutInMs)) {
    bool is_continue = true;

    EmwScopedLock lock(&EmwIoSpi::txLock);

    DEBUG_IO_LOG("\nEmwIoSpi::processPollingData(): %p\n", static_cast<const void *>(EmwIoSpi::txDataAddress))

    if (nullptr == EmwIoSpi::txDataAddress) {
      if (!this->isNotifyHigh()) {
        is_continue = false;
#if defined(EMW_WITH_RTOS)
        if (EmwIoSpi::IoThreadQuitFlag) {
          EmwNetworkStack::freeBuffer(network_buffer_ptr);
          network_buffer_ptr = nullptr;
        }
#endif /* EMW_WITH_RTOS */
      }
    }
    if (is_continue) {
      this->setChipSelectLow();
      if (0 != this->waitFlowHigh()) {
        DRIVER_ERROR_VERBOSE("Wait FLOW timeout 0\n")
      }
      else {
        uint16_t rx_length = 0U;
        if (0 == this->exchangeHeaders(EmwIoSpi::txDataLength, &rx_length)) {
          if (EmwNetworkStack::getBufferPayloadSize(network_buffer_ptr) < rx_length) {
            DEBUG_IO_LOG("EmwIoSpi::processPollingData(): length: %" PRIu32 "-%" PRIu32 "\n",
                         static_cast<uint32_t>(rx_length), static_cast<uint32_t>(EmwIoSpi::txDataLength))
            DRIVER_ERROR_VERBOSE("SPI length invalid\n")
          }
          else {
            uint16_t data_length;
            uint8_t *rx_data_ptr = nullptr;
            if (EmwIoSpi::txDataLength > rx_length) {
              data_length = EmwIoSpi::txDataLength;
            }
            else {
              data_length = rx_length;
            }
            if (0U < rx_length) {
              rx_data_ptr = EmwNetworkStack::getBufferPayload(network_buffer_ptr);
            }
            if (0 != this->waitFlowHigh()) {
              DRIVER_ERROR_VERBOSE("Wait FLOW timeout 1\n")
            }
            else {
              HAL_StatusTypeDef ret;

              if (nullptr != EmwIoSpi::txDataAddress) {
                if (nullptr != rx_data_ptr) {
                  ret = this->transmitReceive(EmwIoSpi::txDataAddress, rx_data_ptr, data_length);
                }
                else {
                  ret = this->transmit(EmwIoSpi::txDataAddress, data_length);
                }
                EmwIoSpi::txDataAddress = nullptr;
                EmwIoSpi::txDataLength = 0U;
              }
              else {
                ret = this->receive(rx_data_ptr, data_length);
              }
              if (HAL_OK != ret) {
                DRIVER_ERROR_VERBOSE("SPI Transmit / Receive data timeout\n")
              }
              else {
                DEBUG_IO_LOG("EmwIoSpi::processPollingData(): slave header length: %" PRIu32 "\n", static_cast<uint32_t>(rx_length))
                if (0U < rx_length) {
                  EmwNetworkStack::setBufferPayloadSize(network_buffer_ptr, rx_length);
                  EmwCoreHci::input(network_buffer_ptr);
                  network_buffer_ptr = nullptr;
                }
              }
            }
          }
        }
      }
      DEBUG_IO_LOG("\nEmwIoSpi::processPollingData()<\n")
    }
    this->setChipSelectHigh();
  }
}

int8_t EmwIoSpi::unInitialize(const class EmwApiCore &coreRef)
{
  (void) this->stop(coreRef);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_RX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_RX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_ERROR_CB_ID);
  return 0;
}

void EmwIoSpi::spiTransferCallback(SPI_HandleTypeDef *spiPtr)
{
  (void) spiPtr;
  (void) EmwOsInterface::releaseSemaphore(&EmwIoSpi::transferDoneSem);
}

void EmwIoSpi::spiErrorCallback(SPI_HandleTypeDef *spiPtr)
{
  (void) spiPtr;
  EmwOsInterface::assertAlways(false);
}

void EmwIoSpi::flowInterruptCallback(void)
{
  (void) EmwOsInterface::releaseSemaphore(&EmwIoSpi::fowRiseSem);
}

void EmwIoSpi::notifyInterruptCallback(void)
{
  (void) EmwOsInterface::releaseSemaphore(&EmwIoSpi::txRxSem);
}

int8_t EmwIoSpi::waitFlowHigh(void)
{
  int8_t status = 0;

  if (EmwOsInterface::eOK != EmwOsInterface::takeSemaphore(&EmwIoSpi::fowRiseSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS)) {
    status = -1;
  }
  if (this->isFlowLow()) {
    DRIVER_ERROR_VERBOSE("FLOW is low\n")
    status = -1;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::WaitFlowHigh()< %" PRIi32 "\n\n", static_cast<int32_t>(status))
  return status;
}

HAL_StatusTypeDef EmwIoSpi::receive(uint8_t *rxDataPtr, uint16_t rxDataLength)
{
  HAL_StatusTypeDef status;

  DEBUG_IO_LOG("\nEmwIoSpi::receive()> %" PRIu32 "\n", static_cast<uint32_t>(rxDataLength))

  status = HAL_SPI_Receive_DMA(this->configuration.hSpiPtr, rxDataPtr, rxDataLength);
  (void) EmwOsInterface::takeSemaphore(&EmwIoSpi::transferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);
#if defined(DEBUG_DETAILS_IO_LOG)
  for (uint32_t i = 0U; i < rxDataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<uint32_t>(*(rxDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */
  DEBUG_IO_LOG("\nEmwIoSpi::receive()< %" PRIi32 "\n\n", static_cast<int32_t>(status))
  return status;
}

uint16_t EmwIoSpi::send(const class EmwApiCore &core, const uint8_t *dataPtr, uint16_t dataLength)
{
  uint16_t sent;
  static_cast<void>(core);
  DEBUG_IO_LOG("\nEmwIoSpi::send()> %" PRIu32 "\n\n", static_cast<uint32_t>(dataLength))

  if (EmwIoSpi::SPI_MAX_BYTE_COUNT < dataLength) {
    DEBUG_IO_LOG("length=%" PRIu32 "\n", static_cast<uint32_t>(dataLength))
    DRIVER_ERROR_VERBOSE("Warning, SPI size overflow!\n")
    sent = 0U;
  }
  else {
    EmwScopedLock lock(&EmwIoSpi::txLock);
    EmwIoSpi::txDataAddress = dataPtr;
    EmwIoSpi::txDataLength = dataLength;
    if (EmwOsInterface::eOK != EmwOsInterface::releaseSemaphore(&EmwIoSpi::txRxSem)) {
      DRIVER_ERROR_VERBOSE("Warning, SPI semaphore has been already notified\n")
    }
    sent = dataLength;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::send()< %" PRIi32 "\n\n", static_cast<int32_t>(sent))
  return sent;
}

HAL_StatusTypeDef EmwIoSpi::transmit(const uint8_t *txDataPtr, uint16_t dataLength)
{
  HAL_StatusTypeDef status;
  DEBUG_IO_LOG("\nEmwIoSpi::transmit()> %" PRIu32 "\n", static_cast<uint32_t>(dataLength))

#if defined(DEBUG_DETAILS_IO_LOG)
  for (uint32_t i = 0U; i < dataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<uint32_t>(*(txDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */

  status = HAL_SPI_Transmit_DMA(this->configuration.hSpiPtr, txDataPtr, dataLength);
  (void) EmwOsInterface::takeSemaphore(&EmwIoSpi::transferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);
  DEBUG_IO_LOG("\nEmwIoSpi::transmit()<%" PRIi32 "\n\n", static_cast<int32_t>(status))
  return status;
}

HAL_StatusTypeDef EmwIoSpi::transmitReceive(const uint8_t *txDataPtr, uint8_t *pRxData, uint16_t dataLength)
{
  HAL_StatusTypeDef ret;
  DEBUG_IO_LOG("\nEmwIoSpi::transmitReceive()> %" PRIu32 "\n", static_cast<uint32_t>(dataLength))

#if defined(DEBUG_DETAILS_IO_LOG)
  for (uint32_t i = 0U; i < dataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<uint32_t>(*(txDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */

  ret = HAL_SPI_TransmitReceive_DMA(this->configuration.hSpiPtr, txDataPtr, pRxData, dataLength);
  (void) EmwOsInterface::takeSemaphore(&EmwIoSpi::transferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);
  DEBUG_IO_LOG("\nEmwIoSpi::transmitReceive()< %" PRIi32 "\n\n", static_cast<int32_t>(ret))
  return ret;
}

int32_t EmwIoSpi::exchangeHeaders(uint16_t txLength, uint16_t *rxLengthPtr)
{
  int32_t ret = -1;
  const uint8_t SPI_WRITE = 0x0AU;
  const uint8_t SPI_READ = 0x0BU;
  const SpiHeader_t spi_master_header = {SPI_WRITE, txLength, static_cast<uint16_t>(~txLength), {0U, 0U, 0U}};
  SpiHeader_t spi_slave_header = {0U, 0U, 0U, {0U, 0U, 0U}};

  DEBUG_IO_LOG("\nEmwIoSpi::exchangeHeaders()>\n")

  if (HAL_OK != this->transmitReceive(reinterpret_cast<const uint8_t *>(&spi_master_header),
                                      reinterpret_cast<uint8_t *>(&spi_slave_header), sizeof(spi_master_header))) {
    DRIVER_ERROR_VERBOSE("Send SPI master header error\n")
  }
  else if (SPI_READ != spi_slave_header.type) {
    DEBUG_IO_LOG("EmwIoSpi::ExchangeHeaders(): type %02x\n", spi_slave_header.type)
    DRIVER_ERROR_VERBOSE("Invalid SPI slave header type\n")
  }
  else if (UINT16_MAX != (static_cast<uint16_t>(spi_slave_header.len ^ spi_slave_header.lenx))) {
    DEBUG_IO_LOG("EmwIoSpi::ExchangeHeaders(): length %04" PRIx32 "-%04" PRIx32 "\n",
                 static_cast<uint32_t>(spi_slave_header.len), static_cast<uint32_t>(spi_slave_header.lenx))
    DRIVER_ERROR_VERBOSE("Invalid SPI slave length\n")
  }
  else if ((0U == spi_master_header.len) && (0U == spi_slave_header.len)) {
    DEBUG_IO_LOG("EmwIoSpi::ExchangeHeaders(): slave header length: %" PRIu32 "\n",
                 static_cast<uint32_t>(spi_slave_header.len))
  }
  else {
    *rxLengthPtr = spi_slave_header.len;
    ret = 0;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::ExchangeHeaders()<\n\n")
  return ret;
}

bool EmwIoSpi::isNotifyHigh(void)
{
  return GPIO_PIN_SET == HAL_GPIO_ReadPin(configuration.hNotifyPortPtr, configuration.notifyPin);
}

bool EmwIoSpi::isFlowLow(void)
{
  return GPIO_PIN_RESET == HAL_GPIO_ReadPin(configuration.hFlowPortPtr, configuration.flowPin);
}

void EmwIoSpi::resetHardware(void) const
{
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::resetHardware()>\n", HAL_GetTick())

  HAL_GPIO_WritePin(this->configuration.hResetPortPtr, this->configuration.resetPin, GPIO_PIN_RESET);
  HAL_Delay(100U);
  HAL_GPIO_WritePin(this->configuration.hResetPortPtr, this->configuration.resetPin, GPIO_PIN_SET);
  HAL_Delay(1200U);
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::resetHardware()<\n\n", HAL_GetTick())
}

void EmwIoSpi::setChipSelectHigh(void) const
{
  HAL_GPIO_WritePin(this->configuration.hNssPortPtr, this->configuration.nssPin, GPIO_PIN_SET);
}

void EmwIoSpi::setChipSelectLow(void) const
{
  HAL_GPIO_WritePin(this->configuration.hNssPortPtr, this->configuration.nssPin, GPIO_PIN_RESET);
}

void EmwIoSpi::start(const class EmwApiCore &core)
{
#if !defined(EMW_WITH_RTOS)
  static_cast<void>(core);
#endif /* EMW_WITH_RTOS */
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::start()>\n", HAL_GetTick())

  {
    static const char tx_lock_name[] = {"EMW-SpiTxLock"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createMutex(&EmwIoSpi::txLock, tx_lock_name);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char txrx_sem_name[] = {"EMW-SpiTxRxSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createSemaphore(&EmwIoSpi::txRxSem, txrx_sem_name, 2U, 0U);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char flow_rise_sem_name[] = {"EMW-SpiFlowRiseSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createSemaphore(&EmwIoSpi::fowRiseSem,
      flow_rise_sem_name, 1U, 0U);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char transfer_done_sem_name[] = {"EMW-SpiTransferDoneSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createSemaphore(&EmwIoSpi::transferDoneSem,
      transfer_done_sem_name, 1U, 0U);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_RTOS)
  {
    static const char io_thread_name[] = {"EMW-SPI_DMA_Thread"};
    const EmwOsInterface::Status os_status = EmwOsInterface::createThread(&IoThread, io_thread_name,
      IoThreadFunction, reinterpret_cast<EmwOsInterface::ThreadFunctionArgument_t>(&core),
      EMW_IO_SPI_THREAD_STACK_SIZE, EMW_IO_SPI_THREAD_PRIORITY);
    EmwOsInterface::assertAlways(EmwOsInterface::eOK == os_status);
    /* Be cooperative. */
    (void) EmwOsInterface::delayTicks(1U);
  }
#endif /* EMW_WITH_RTOS */
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_ERROR_CB_ID, EmwIoSpi::spiErrorCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_RX_COMPLETE_CB_ID, EmwIoSpi::spiTransferCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_COMPLETE_CB_ID, EmwIoSpi::spiTransferCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_RX_COMPLETE_CB_ID,
                                  EmwIoSpi::spiTransferCallback);
  (void) HAL_EXTI_RegisterCallback(this->configuration.hExtiFlowPtr, HAL_EXTI_RISING_CB_ID,
                                   EmwIoSpi::flowInterruptCallback);
  (void) HAL_EXTI_RegisterCallback(this->configuration.hExtiNotifyPtr, HAL_EXTI_RISING_CB_ID,
                                   EmwIoSpi::notifyInterruptCallback);
  this->setChipSelectHigh();

  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::start()<\n\n", HAL_GetTick())
}

int8_t EmwIoSpi::stop(const class EmwApiCore &core)
{
  (void)core;
  DEBUG_IO_LOG("\nEmwIoSpi::stop()>\n")

#if defined(EMW_WITH_RTOS)
  EmwIoSpi::IoThreadQuitFlag = true;
#endif /* EMW_WITH_RTOS */
  (void) EmwOsInterface::releaseSemaphore(&EmwIoSpi::txRxSem);

#if defined(EMW_WITH_RTOS)
  while (EmwIoSpi::IoThreadQuitFlag) {
    (void) EmwOsInterface::delayTicks(50U);
  }
#endif /* EMW_WITH_RTOS */

  /* TODO: check if interrupt are to be disabled */
#if defined(EMW_WITH_RTOS)
  (void) EmwOsInterface::terminateThread(&IoThread);
  (void) EmwOsInterface::delayTicks(1U);
#endif /* EMW_WITH_RTOS */

  (void) EmwOsInterface::deleteSemaphore(&EmwIoSpi::transferDoneSem);
  (void) EmwOsInterface::deleteSemaphore(&EmwIoSpi::fowRiseSem);
  (void) EmwOsInterface::deleteSemaphore(&EmwIoSpi::txRxSem);
  (void) EmwOsInterface::deleteMutex(&EmwIoSpi::txLock);
  DEBUG_IO_LOG("\nEmwIoSpi::stop()<\n\n")
  return 0;
}

#if defined(EMW_WITH_RTOS)
EmwOsInterface::Thread_t EmwIoSpi::IoThread;
volatile bool EmwIoSpi::IoThreadQuitFlag = false;

void EmwIoSpi::IoThreadFunction(EmwOsInterface::ThreadFunctionArgument_t argument)
{
  const class EmwApiCore *const core_ptr = static_cast<const class EmwApiCore *>(argument);
  DEBUG_IO_LOG("\n[%" PRIu32 "] EmwIoSpi::IoThreadFunction()>\n", HAL_GetTick())

#if defined(EMW_IO_DEBUG)
  std::setbuf(stdout, nullptr);
#endif /* EMW_IO_DEBUG */

  EmwIoSpi::IoThreadQuitFlag = false;
  while (!EmwIoSpi::IoThreadQuitFlag) {
    core_ptr->ioPtr->processPollingData(*core_ptr, EMW_OS_TIMEOUT_FOREVER);
  }
  EmwIoSpi::IoThreadQuitFlag = false;
  (void) EmwOsInterface::exitThread();
}
#endif /* EMW_WITH_RTOS */

EmwOsInterface::Semaphore_t EmwIoSpi::fowRiseSem;
EmwOsInterface::Semaphore_t EmwIoSpi::transferDoneSem;
const uint8_t *EmwIoSpi::txDataAddress = nullptr;
uint16_t EmwIoSpi::txDataLength = 0U;
EmwOsInterface::Mutex_t EmwIoSpi::txLock;
EmwOsInterface::Semaphore_t EmwIoSpi::txRxSem;
