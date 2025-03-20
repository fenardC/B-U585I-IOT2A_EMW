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
#include <cstdint>
#include <cstring>

#if !defined(EMW_IO_DEBUG)
#define DEBUG_IO_LOG(...)
#define DEBUG_IO_WARNING(...)
#else
#define DEBUG_DETAILS_IO_LOG
#endif /* EMW_IO_DEBUG */

typedef __PACKED_STRUCT SpiHeader_s{
  constexpr SpiHeader_s(void) noexcept
    : type(0U), len(0U), lenx(0U), dummy{0U} {}
  constexpr explicit SpiHeader_s(std::uint8_t type, std::uint16_t length) noexcept
    : type(type), len(length), lenx(static_cast<std::uint16_t>(~length)), dummy{0U} {}

  std::uint8_t type;
  std::uint16_t len;
  std::uint16_t lenx;
  std::uint8_t dummy[3];
} SpiHeader_t;

EmwIoSpi::EmwIoSpi(void) noexcept
  : configuration()
{
  DEBUG_IO_LOG("\nEmwIoSpi::EmwIoSpi()<\n")
}

EmwIoSpi::~EmwIoSpi(void) noexcept
{
  DEBUG_IO_LOG("\nEmwIoSpi::~EmwIoSpi()<\n")
}

void EmwIoSpi::initializeImp(EmwIoInterfaceTypes::InitializationMode mode) noexcept
{
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::initializeImp()>\n", HAL_GetTick())

  if (EmwIoInterfaceTypes::eRESET == mode) {
    const std::int32_t status = EmwIoHardware::initialize(this->configuration);

    EmwOsInterface::AssertAlways(0 == status);
    this->setChipSelectHigh();
    this->resetHardware();
  }
  else {
    this->start();
  }
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::initializeImp()<\n\n", HAL_GetTick())
}

void EmwIoSpi::pollDataImp(std::uint32_t timeoutInMs) noexcept
{
#if defined(EMW_WITH_NO_OS)
  this->processPollingDataImp(timeoutInMs);
#else
  static_cast<void>(timeoutInMs);
#endif /* EMW_WITH_NO_OS */
}

void EmwIoSpi::processPollingDataImp(std::uint32_t timeoutInMs) noexcept
{
  static EmwNetworkStack::Buffer_t *network_buffer_ptr = nullptr;
  bool first_miss = true;

  this->setChipSelectHigh();

  while (nullptr == network_buffer_ptr) {
    network_buffer_ptr = EmwNetworkStack::AllocBuffer();
    if (nullptr == network_buffer_ptr) {
#if defined(EMW_WITH_RTOS)
      /* Be cooperative */
      EmwOsInterface::DelayTicks(1U);
#endif /* EMW_WITH_RTOS */
      if (first_miss) {
        first_miss = false;
        DEBUG_IO_WARNING("Running out of buffer for RX\n")
      }
    }
  }
  if (EmwOsInterface::eOK == EmwOsInterface::TakeSemaphore(EmwIoSpi::TxRxSem, timeoutInMs)) {
    bool is_continue = true;

    EmwScopedLock lock(EmwIoSpi::TxLock);

    DEBUG_IO_LOG("\nEmwIoSpi::processPollingDataImp(): %p\n", static_cast<const void *>(EmwIoSpi::TxDataAddress))

    if (nullptr == EmwIoSpi::TxDataAddress) {
      if (!this->isNotifyHigh()) {
        is_continue = false;
#if defined(EMW_WITH_RTOS)
        if (EmwIoSpi::IoThreadQuitFlag) {
          EmwNetworkStack::FreeBuffer(network_buffer_ptr);
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
        std::uint16_t rx_length = 0U;
        if (0 == this->exchangeHeaders(EmwIoSpi::TxDataLength, rx_length)) {
          if (EmwNetworkStack::GetBufferPayloadSize(network_buffer_ptr) < rx_length) {
            DEBUG_IO_LOG("EmwIoSpi::processPollingDataImp(): length: %" PRIu32 "-%" PRIu32 "\n",
                         static_cast<std::uint32_t>(rx_length), static_cast<std::uint32_t>(EmwIoSpi::TxDataLength))
            DRIVER_ERROR_VERBOSE("SPI length invalid\n")
          }
          else {
            std::uint16_t data_length;
            std::uint8_t *rx_data_ptr = nullptr;
            if (EmwIoSpi::TxDataLength > rx_length) {
              data_length = EmwIoSpi::TxDataLength;
            }
            else {
              data_length = rx_length;
            }
            if (0U < rx_length) {
              rx_data_ptr = EmwNetworkStack::GetBufferPayload(network_buffer_ptr);
            }
            if (0 != this->waitFlowHigh()) {
              DRIVER_ERROR_VERBOSE("Wait FLOW timeout 1\n")
            }
            else {
              HAL_StatusTypeDef ret;

              if (nullptr != EmwIoSpi::TxDataAddress) {
                if (nullptr != rx_data_ptr) {
                  ret = this->transmitReceive(EmwIoSpi::TxDataAddress, rx_data_ptr, data_length);
                }
                else {
                  ret = this->transmit(EmwIoSpi::TxDataAddress, data_length);
                }
                EmwIoSpi::TxDataAddress = nullptr;
                EmwIoSpi::TxDataLength = 0U;
              }
              else {
                ret = this->receive(rx_data_ptr, data_length);
              }
              if (HAL_OK != ret) {
                DRIVER_ERROR_VERBOSE("SPI Transmit / Receive data timeout\n")
              }
              else {
                DEBUG_IO_LOG("EmwIoSpi::processPollingDataImp(): slave header length: %" PRIu32 "\n",
                             static_cast<std::uint32_t>(rx_length))

                if (0U < rx_length) {
                  EmwNetworkStack::SetBufferPayloadSize(network_buffer_ptr, rx_length);
                  EmwCoreHci::Input(network_buffer_ptr);
                  network_buffer_ptr = nullptr;
                }
              }
            }
          }
        }
      }
      DEBUG_IO_LOG("\nEmwIoSpi::processPollingDataImp()<\n")
    }
    this->setChipSelectHigh();
  }
}

std::uint16_t EmwIoSpi::sendImp(const std::uint8_t *dataPtr, std::uint16_t dataLength) noexcept
{
  std::uint16_t sent;

  DEBUG_IO_LOG("\nEmwIoSpi::sendImp()> %" PRIu32 "\n\n", static_cast<std::uint32_t>(dataLength))

  if (EmwIoSpi::SPI_MAX_BYTE_COUNT < dataLength) {
    DEBUG_IO_LOG("length=%" PRIu32 "\n", static_cast<std::uint32_t>(dataLength))
    DRIVER_ERROR_VERBOSE("Warning, SPI size overflow!\n")
    sent = 0U;
  }
  else {
    EmwScopedLock lock(EmwIoSpi::TxLock);

    EmwIoSpi::TxDataAddress = dataPtr;
    EmwIoSpi::TxDataLength = dataLength;
    if (EmwOsInterface::eOK != EmwOsInterface::ReleaseSemaphore(EmwIoSpi::TxRxSem)) {
      DRIVER_ERROR_VERBOSE("Warning, SPI semaphore has been already notified\n")
    }
    sent = dataLength;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::sendImp()< %" PRIi32 "\n\n", static_cast<std::int32_t>(sent))
  return sent;
}

int8_t EmwIoSpi::unInitializeImp(void) noexcept
{
  this->stop();
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_RX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_RX_COMPLETE_CB_ID);
  (void) HAL_SPI_UnRegisterCallback(this->configuration.hSpiPtr, HAL_SPI_ERROR_CB_ID);
  return 0;
}


void EmwIoSpi::SpiTransferCallback(SPI_HandleTypeDef *spiPtr) noexcept
{
  static_cast<void>(spiPtr);
  (void) EmwOsInterface::ReleaseSemaphore(EmwIoSpi::TransferDoneSem);
}

void EmwIoSpi::SpiErrorCallback(SPI_HandleTypeDef *spiPtr) noexcept
{
  static_cast<void>(spiPtr);
  EmwOsInterface::AssertAlways(false);
}

void EmwIoSpi::FlowInterruptCallback(void) noexcept
{
  (void) EmwOsInterface::ReleaseSemaphore(EmwIoSpi::FowRiseSem);
}

void EmwIoSpi::NotifyInterruptCallback(void) noexcept
{
  (void) EmwOsInterface::ReleaseSemaphore(EmwIoSpi::TxRxSem);
}

std::int8_t EmwIoSpi::waitFlowHigh(void) noexcept
{
  std::int8_t status = 0;

  if (EmwOsInterface::eOK != EmwOsInterface::TakeSemaphore(EmwIoSpi::FowRiseSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS)) {
    status = -1;
  }
  if (this->isFlowLow()) {
    DRIVER_ERROR_VERBOSE("FLOW is low\n")
    status = -1;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::WaitFlowHigh()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

HAL_StatusTypeDef EmwIoSpi::receive(std::uint8_t *rxDataPtr, std::uint16_t rxDataLength) noexcept
{
  HAL_StatusTypeDef status;

  DEBUG_IO_LOG("\nEmwIoSpi::receive()> %" PRIu32 "\n", static_cast<std::uint32_t>(rxDataLength))

  status = HAL_SPI_Receive_DMA(this->configuration.hSpiPtr, rxDataPtr, rxDataLength);
  (void) EmwOsInterface::TakeSemaphore(EmwIoSpi::TransferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);

#if defined(DEBUG_DETAILS_IO_LOG)
  for (std::uint32_t i = 0U; i < rxDataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<std::uint32_t>(*(rxDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */
  DEBUG_IO_LOG("\nEmwIoSpi::receive()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

HAL_StatusTypeDef EmwIoSpi::transmit(const std::uint8_t *txDataPtr, std::uint16_t dataLength) noexcept
{
  HAL_StatusTypeDef status;

  DEBUG_IO_LOG("\nEmwIoSpi::transmit()> %" PRIu32 "\n", static_cast<std::uint32_t>(dataLength))

#if defined(DEBUG_DETAILS_IO_LOG)
  for (std::uint16_t i = 0U; i < dataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<std::uint32_t>(*(txDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */

  status = HAL_SPI_Transmit_DMA(this->configuration.hSpiPtr, txDataPtr, dataLength);
  (void) EmwOsInterface::TakeSemaphore(EmwIoSpi::TransferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);
  DEBUG_IO_LOG("\nEmwIoSpi::transmit()<%" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

HAL_StatusTypeDef EmwIoSpi::transmitReceive(const std::uint8_t *txDataPtr, std::uint8_t *rxDataPtr,
    std::uint16_t dataLength) noexcept
{
  HAL_StatusTypeDef status;

  DEBUG_IO_LOG("\nEmwIoSpi::transmitReceive()> %" PRIu32 "\n", static_cast<std::uint32_t>(dataLength))

#if defined(DEBUG_DETAILS_IO_LOG)
  for (std::uint16_t i = 0U; i < dataLength; i++) {
    DEBUG_IO_LOG("%02" PRIx32 " ", static_cast<std::uint32_t>(*(txDataPtr + i)))
  }
#endif /* DEBUG_DETAILS_IO_LOG */

  status = HAL_SPI_TransmitReceive_DMA(this->configuration.hSpiPtr, txDataPtr, rxDataPtr, dataLength);
  (void) EmwOsInterface::TakeSemaphore(EmwIoSpi::TransferDoneSem, EmwIoSpi::TIMEOUT_HARDWARE_EMW_MS);
  DEBUG_IO_LOG("\nEmwIoSpi::transmitReceive()< %" PRIi32 "\n\n", static_cast<std::int32_t>(status))
  return status;
}

std::int32_t EmwIoSpi::exchangeHeaders(std::uint16_t txLength, std::uint16_t &rxLength) noexcept
{
  std::int32_t status = -1;
  const std::uint8_t SPI_WRITE = 0x0AU;
  const std::uint8_t SPI_READ = 0x0BU;
  const SpiHeader_t spi_master_header(SPI_WRITE, txLength);
  SpiHeader_t spi_slave_header;

  DEBUG_IO_LOG("\nEmwIoSpi::exchangeHeaders()>\n")

  if (HAL_OK != this->transmitReceive(reinterpret_cast<const std::uint8_t *>(&spi_master_header),
                                      reinterpret_cast<std::uint8_t *>(&spi_slave_header), sizeof(spi_master_header))) {
    DRIVER_ERROR_VERBOSE("Send SPI master header error\n")
  }
  else if (SPI_READ != spi_slave_header.type) {
    DEBUG_IO_LOG("EmwIoSpi::exchangeHeaders(): type %02x\n", spi_slave_header.type)
    DRIVER_ERROR_VERBOSE("Invalid SPI slave header type\n")
  }
  else if (UINT16_MAX != (static_cast<std::uint16_t>(spi_slave_header.len ^ spi_slave_header.lenx))) {
    DEBUG_IO_LOG("EmwIoSpi::exchangeHeaders(): length %04" PRIx32 "-%04" PRIx32 "\n",
                 static_cast<std::uint32_t>(spi_slave_header.len), static_cast<std::uint32_t>(spi_slave_header.lenx))
    DRIVER_ERROR_VERBOSE("Invalid SPI slave length\n")
  }
  else if ((0U == spi_master_header.len) && (0U == spi_slave_header.len)) {
    DEBUG_IO_LOG("EmwIoSpi::exchangeHeaders(): slave header length: %" PRIu32 "\n",
                 static_cast<std::uint32_t>(spi_slave_header.len))
  }
  else {
    rxLength = spi_slave_header.len;
    status = 0;
  }
  DEBUG_IO_LOG("\nEmwIoSpi::exchangeHeaders()<\n\n")
  return status;
}

bool EmwIoSpi::isNotifyHigh(void) noexcept
{
  return GPIO_PIN_SET == HAL_GPIO_ReadPin(configuration.hNotifyPortPtr, configuration.notifyPin);
}

bool EmwIoSpi::isFlowLow(void) noexcept
{
  return GPIO_PIN_RESET == HAL_GPIO_ReadPin(configuration.hFlowPortPtr, configuration.flowPin);
}

void EmwIoSpi::resetHardware(void) const noexcept
{
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::resetHardware()>\n", HAL_GetTick())

  HAL_GPIO_WritePin(this->configuration.hResetPortPtr, this->configuration.resetPin, GPIO_PIN_RESET);
  HAL_Delay(100U);
  HAL_GPIO_WritePin(this->configuration.hResetPortPtr, this->configuration.resetPin, GPIO_PIN_SET);
  HAL_Delay(1200U);
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::resetHardware()<\n\n", HAL_GetTick())
}

void EmwIoSpi::setChipSelectHigh(void) const noexcept
{
  HAL_GPIO_WritePin(this->configuration.hNssPortPtr, this->configuration.nssPin, GPIO_PIN_SET);
}

void EmwIoSpi::setChipSelectLow(void) const noexcept
{
  HAL_GPIO_WritePin(this->configuration.hNssPortPtr, this->configuration.nssPin, GPIO_PIN_RESET);
}

void EmwIoSpi::start(void) noexcept
{
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::start()>\n", HAL_GetTick())

  {
    static const char tx_lock_name[] = {"EMW-SpiTxLock"};
    const EmwOsInterface::Status os_status = EmwOsInterface::CreateMutex(EmwIoSpi::TxLock, tx_lock_name);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char txrx_sem_name[] = {"EMW-SpiTxRxSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::CreateSemaphore(EmwIoSpi::TxRxSem, txrx_sem_name, 2U, 0U);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char flow_rise_sem_name[] = {"EMW-SpiFlowRiseSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::CreateSemaphore(EmwIoSpi::FowRiseSem,
      flow_rise_sem_name, 1U, 0U);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
  {
    static const char transfer_done_sem_name[] = {"EMW-SpiTransferDoneSem"};
    const EmwOsInterface::Status os_status = EmwOsInterface::CreateSemaphore(EmwIoSpi::TransferDoneSem,
      transfer_done_sem_name, 1U, 0U);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
  }
#if defined(EMW_WITH_RTOS)
  {
    static const char io_thread_name[] = {"EMW-SPI_DMA_Thread"};
    const EmwOsInterface::Status os_status \
      = EmwOsInterface::CreateThread(EmwIoSpi::IoThread, io_thread_name, EmwIoSpi::IoThreadFunction, this,
                                     EMW_IO_SPI_THREAD_STACK_SIZE, EMW_IO_SPI_THREAD_PRIORITY);
    EmwOsInterface::AssertAlways(EmwOsInterface::eOK == os_status);
    /* Be cooperative. */
    EmwOsInterface::DelayTicks(1U);
  }
#endif /* EMW_WITH_RTOS */
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_ERROR_CB_ID, EmwIoSpi::SpiErrorCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_RX_COMPLETE_CB_ID, EmwIoSpi::SpiTransferCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_COMPLETE_CB_ID, EmwIoSpi::SpiTransferCallback);
  (void) HAL_SPI_RegisterCallback(this->configuration.hSpiPtr, HAL_SPI_TX_RX_COMPLETE_CB_ID,
                                  EmwIoSpi::SpiTransferCallback);
  (void) HAL_EXTI_RegisterCallback(this->configuration.hExtiFlowPtr, HAL_EXTI_RISING_CB_ID,
                                   EmwIoSpi::FlowInterruptCallback);
  (void) HAL_EXTI_RegisterCallback(this->configuration.hExtiNotifyPtr, HAL_EXTI_RISING_CB_ID,
                                   EmwIoSpi::NotifyInterruptCallback);
  this->setChipSelectHigh();

  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::start()<\n\n", HAL_GetTick())
}

void EmwIoSpi::stop(void) noexcept
{
  DEBUG_IO_LOG("\nEmwIoSpi::stop()>\n")

#if defined(EMW_WITH_RTOS)
  EmwIoSpi::IoThreadQuitFlag = true;
#endif /* EMW_WITH_RTOS */
  (void) EmwOsInterface::ReleaseSemaphore(EmwIoSpi::TxRxSem);

#if defined(EMW_WITH_RTOS)
  while (EmwIoSpi::IoThreadQuitFlag) {
    EmwOsInterface::DelayTicks(50U);
  }
#endif /* EMW_WITH_RTOS */

  /* TODO: check if interrupt are to be disabled */
#if defined(EMW_WITH_RTOS)
  EmwOsInterface::TerminateThread(IoThread);
  EmwOsInterface::DelayTicks(1U);
#endif /* EMW_WITH_RTOS */

  (void) EmwOsInterface::DeleteSemaphore(EmwIoSpi::TransferDoneSem);
  (void) EmwOsInterface::DeleteSemaphore(EmwIoSpi::FowRiseSem);
  (void) EmwOsInterface::DeleteSemaphore(EmwIoSpi::TxRxSem);
  (void) EmwOsInterface::DeleteMutex(EmwIoSpi::TxLock);
  DEBUG_IO_LOG("\nEmwIoSpi::stop()<\n\n")
}

#if defined(EMW_WITH_RTOS)
EmwOsInterface::Thread_t EmwIoSpi::IoThread;
volatile bool EmwIoSpi::IoThreadQuitFlag = false;

void EmwIoSpi::IoThreadFunction(EmwOsInterface::ThreadFunctionArgument_t argumentPtr) noexcept
{
#if defined(EMW_IO_DEBUG)
  std::setbuf(stdout, nullptr);
  DEBUG_IO_LOG("\n[%6" PRIu32 "] EmwIoSpi::IoThreadFunction()>\n", HAL_GetTick())
#endif /* EMW_IO_DEBUG */
  {
    EmwIoSpi * const THIS = static_cast<EmwIoSpi *>(const_cast<void *>(argumentPtr));

    EmwIoSpi::IoThreadQuitFlag = false;
    while (!EmwIoSpi::IoThreadQuitFlag) {
      THIS->processPollingData(EMW_OS_TIMEOUT_FOREVER);
    }
    EmwIoSpi::IoThreadQuitFlag = false;
    EmwOsInterface::ExitThread();
  }
}
#endif /* EMW_WITH_RTOS */

EmwOsInterface::Semaphore_t EmwIoSpi::FowRiseSem;
EmwOsInterface::Semaphore_t EmwIoSpi::TransferDoneSem;
const std::uint8_t *EmwIoSpi::TxDataAddress = nullptr;
std::uint16_t EmwIoSpi::TxDataLength = 0U;
EmwOsInterface::Mutex_t EmwIoSpi::TxLock;
EmwOsInterface::Semaphore_t EmwIoSpi::TxRxSem;
