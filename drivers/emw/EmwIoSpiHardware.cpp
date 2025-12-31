/**
  ******************************************************************************
  * Copyright (C) 2025-2026 C.Fenard.
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
#include "EmwIoHardware.hpp"
#include "spi.h"

static constexpr uint16_t EMW_FLOW_Pin = 0x8000U;
#define EMW_FLOW_GPIO_Port reinterpret_cast<GPIO_TypeDef *>(GPIOG_BASE_NS)
#define EMW_FLOW_EXTI_IRQn EXTI15_IRQn

static constexpr uint16_t EMW_NOTIFY_Pin = 0x4000U;
#define EMW_NOTIFY_GPIO_Port reinterpret_cast<GPIO_TypeDef *>(GPIOD_BASE_NS)
#define EMW_NOTIFY_EXTI_IRQn EXTI14_IRQn

static constexpr uint16_t EMW_NSS_Pin = 0x1000U;
#define EMW_NSS_GPIO_Port reinterpret_cast<GPIO_TypeDef *>(GPIOB_BASE_NS)

static constexpr uint16_t EMW_RESET_Pin = 0x8000U;
#define EMW_RESET_GPIO_Port reinterpret_cast<GPIO_TypeDef *>(GPIOF_BASE_NS)

static EXTI_HandleTypeDef *getExtiFlow(void) noexcept;
static EXTI_HandleTypeDef *getExtiNotify(void) noexcept;


static EXTI_HandleTypeDef *getExtiFlow(void) noexcept
{
  static EXTI_HandleTypeDef external_interrupt =
  {EMW_FLOW_EXTI_IRQn, nullptr, nullptr};
  return &external_interrupt;
}

static EXTI_HandleTypeDef *getExtiNotify(void) noexcept
{
  static EXTI_HandleTypeDef external_interrupt =
  {EMW_NOTIFY_EXTI_IRQn, nullptr, nullptr};
  return &external_interrupt;
}

std::int32_t EmwIoHardware::initialize(EmwIoSpi::Stm32Hw_s &configuration) noexcept
{
  std::int32_t status;

#if defined(EMW_USE_SPI_DMA)
  configuration.hSpiPtr = &hSpi2;
  configuration.hNssPortPtr = EMW_NSS_GPIO_Port;
  configuration.nssPin = EMW_NSS_Pin;
  configuration.hExtiFlowPtr = getExtiFlow();
  configuration.hFlowPortPtr = EMW_FLOW_GPIO_Port;
  configuration.flowPin = EMW_FLOW_Pin;
  configuration.flowIrq = EXTI15_IRQn;
  configuration.hExtiNotifyPtr = getExtiNotify();
  configuration.hNotifyPortPtr = EMW_NOTIFY_GPIO_Port;
  configuration.notifyPin = EMW_NOTIFY_Pin;
  configuration.notifyIrq = EXTI14_IRQn;
  configuration.hResetPortPtr = EMW_RESET_GPIO_Port;
  configuration.resetPin = EMW_RESET_Pin;
  status = 0;
#else
  status = -1;
#endif /* EMW_USE_SPI_DMA */

  return status;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin)
{
  switch (pin) {
    case (EMW_FLOW_Pin): {
        EXTI_HandleTypeDef *const p = getExtiFlow();
        if (nullptr != p->RisingCallback) {
          p->RisingCallback();
        }
        break;
      }
    case (EMW_NOTIFY_Pin): {
        EXTI_HandleTypeDef *const p = getExtiNotify();
        if (nullptr != p->RisingCallback) {
          p->RisingCallback();
        }
        break;
      }
    default: {
        break;
      }
  }
}
