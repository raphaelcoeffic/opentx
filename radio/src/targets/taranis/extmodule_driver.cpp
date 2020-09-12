/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "board_common.h"

void extmoduleStop()
{
  NVIC_DisableIRQ(EXTMODULE_TIMER_DMA_STREAM_IRQn);
  NVIC_DisableIRQ(EXTMODULE_TIMER_CC_IRQn);
  EXTMODULE_TIMER_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA

#if defined(EXTMODULE_USART)
  EXTMODULE_USART_TX_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN | EXTMODULE_RX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(EXTMODULE_USART_GPIO, &GPIO_InitStructure);

  GPIO_ResetBits(EXTMODULE_USART_GPIO, EXTMODULE_TX_GPIO_PIN | EXTMODULE_RX_GPIO_PIN);
#endif

  EXTMODULE_TIMER->DIER &= ~(TIM_DIER_CC2IE | TIM_DIER_UDE);
  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;

  if (!IS_TRAINER_EXTERNAL_MODULE()) {
    EXTERNAL_MODULE_PWR_OFF();
  }
}

void extmodulePpmStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PinSource, EXTMODULE_TIMER_TX_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_TX_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS from 30MHz
  EXTMODULE_TIMER->CCR1 = GET_MODULE_PPM_DELAY(EXTERNAL_MODULE)*2;
  EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE | (GET_MODULE_PPM_POLARITY(EXTERNAL_MODULE) ? EXTMODULE_TIMER_OUTPUT_POLARITY : 0); //     // we are using complementary output so logic has to be reversed here
  EXTMODULE_TIMER->BDTR = TIM_BDTR_MOE;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0; // Force O/P high
  EXTMODULE_TIMER->EGR = 1;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC2PE; // PWM mode 1
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF; // Clear flag
  EXTMODULE_TIMER->ARR = 45000;
  EXTMODULE_TIMER->CCR2 = 40000; // The first frame will be sent in 20ms
  EXTMODULE_TIMER->DIER |= TIM_DIER_UDE | TIM_DIER_CC2IE;
  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  NVIC_EnableIRQ(EXTMODULE_TIMER_DMA_STREAM_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_DMA_STREAM_IRQn, 7);
  NVIC_EnableIRQ(EXTMODULE_TIMER_CC_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_CC_IRQn, 7);
}

void extmoduleSerialStart()
{
  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PinSource, EXTMODULE_TIMER_TX_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_TX_GPIO, &GPIO_InitStructure);

  // disable timer
  TIM_Cmd(EXTMODULE_TIMER, DISABLE);

  TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);

  TIM_TimeBaseStruct.TIM_Prescaler = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  TIM_TimeBaseStruct.TIM_Period = 40000; // fake value, big enough for a complete cycle
  TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(EXTMODULE_TIMER, &TIM_TimeBaseStruct);

  TIM_OCInitTypeDef TIM_OCInitStruct;
  TIM_OCStructInit(&TIM_OCInitStruct);

  TIM_OCInitStruct.TIM_Pulse = 0; // whatever, will be replaced by DMA data...
  TIM_OCInitStruct.TIM_OCMode = TIM_ForcedAction_Active;

  // depends on EXTMODULE_TIMER_OUTPUT_ENABLE & EXTMODULE_TIMER_OUTPUT_POLARITY:
  //
  // TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
  // TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable;
  // TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
  // TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;

  TIM_OC1Init(EXTMODULE_TIMER, &TIM_OCInitStruct);

  if (PROTOCOL_CHANNELS_SBUS == moduleState[EXTERNAL_MODULE].protocol) {
    EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE | (GET_SBUS_POLARITY(EXTERNAL_MODULE) ? EXTMODULE_TIMER_OUTPUT_POLARITY : 0); // reverse polarity for Sbus if needed
  }
  else {
    EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE | EXTMODULE_TIMER_OUTPUT_POLARITY;
  }

  TIM_SelectOCxM(EXTMODULE_TIMER, TIM_Channel_1, TIM_OCMode_Toggle);

  TIM_CtrlPWMOutputs(EXTMODULE_TIMER, ENABLE);
  TIM_DMACmd(EXTMODULE_TIMER, TIM_DMA_Update, ENABLE);

  // re-enable timer
  TIM_Cmd(EXTMODULE_TIMER, ENABLE);

  NVIC_EnableIRQ(EXTMODULE_TIMER_DMA_STREAM_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_DMA_STREAM_IRQn, 7);
}

#if defined(EXTMODULE_USART)
ModuleFifo extmoduleFifo;

void extmoduleInvertedSerialStart(uint32_t baudrate)
{
  EXTERNAL_MODULE_ON();

  // TX + RX Pins
  GPIO_PinAFConfig(EXTMODULE_USART_GPIO, EXTMODULE_TX_GPIO_PinSource, EXTMODULE_USART_GPIO_AF);
  GPIO_PinAFConfig(EXTMODULE_USART_GPIO, EXTMODULE_RX_GPIO_PinSource, EXTMODULE_USART_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN | EXTMODULE_RX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(EXTMODULE_USART_GPIO, &GPIO_InitStructure);

  // UART config
  USART_DeInit(EXTMODULE_USART);
  USART_InitTypeDef USART_InitStructure;
  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(EXTMODULE_USART, &USART_InitStructure);
  USART_Cmd(EXTMODULE_USART, ENABLE);

  extmoduleFifo.clear();

  USART_ITConfig(EXTMODULE_USART, USART_IT_RXNE, ENABLE);
  NVIC_SetPriority(EXTMODULE_USART_IRQn, 6);
  NVIC_EnableIRQ(EXTMODULE_USART_IRQn);
}

void extmoduleSendBuffer(const uint8_t * data, uint8_t size)
{
  DMA_InitTypeDef DMA_InitStructure;
  DMA_DeInit(EXTMODULE_USART_TX_DMA_STREAM);

  DMA_InitStructure.DMA_Channel = EXTMODULE_USART_TX_DMA_CHANNEL;
  DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&EXTMODULE_USART->DR);
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_Memory0BaseAddr = CONVERT_PTR_UINT(data);
  DMA_InitStructure.DMA_BufferSize = size;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

  DMA_Init(EXTMODULE_USART_TX_DMA_STREAM, &DMA_InitStructure);
  DMA_Cmd(EXTMODULE_USART_TX_DMA_STREAM, ENABLE);
  USART_DMACmd(EXTMODULE_USART, USART_DMAReq_Tx, ENABLE);
}

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)
extern "C" void EXTMODULE_USART_IRQHandler(void)
{
  uint32_t status = EXTMODULE_USART->SR;

  while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
    uint8_t data = EXTMODULE_USART->DR;
    if (status & USART_FLAG_ERRORS) {
      extmoduleFifo.errors++;
    }
    else {
      extmoduleFifo.push(data);
    }
    status = EXTMODULE_USART->SR;
  }
}
#endif

#if defined(PXX1)
void extmodulePxx1PulsesStart()
{
  EXTERNAL_MODULE_ON();

  // configure output
  GPIO_PinAFConfig(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PinSource, EXTMODULE_TIMER_TX_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_TX_GPIO, &GPIO_InitStructure);

  // disable timer
  TIM_Cmd(EXTMODULE_TIMER, DISABLE);

  TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);

  TIM_TimeBaseStruct.TIM_Prescaler = EXTMODULE_TIMER_FREQ / 2000000 - 1; // 0.5uS (2Mhz)
  TIM_TimeBaseStruct.TIM_Period = 40000; // fake value, big enough for a complete cycle
  TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(EXTMODULE_TIMER, &TIM_TimeBaseStruct);

  TIM_OCInitTypeDef TIM_OCInitStruct;
  TIM_OCStructInit(&TIM_OCInitStruct);

  TIM_OCInitStruct.TIM_Pulse = 18; // 9us

  // depends on EXTMODULE_TIMER_OUTPUT_ENABLE & EXTMODULE_TIMER_OUTPUT_POLARITY:
  //
  // TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
  // TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable;
  // TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
  // TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;

  TIM_OC1Init(EXTMODULE_TIMER, &TIM_OCInitStruct);

  EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE | EXTMODULE_TIMER_OUTPUT_POLARITY;
  TIM_SelectOCxM(EXTMODULE_TIMER, TIM_Channel_1, TIM_OCMode_PWM1);
  TIM_OC1PreloadConfig(EXTMODULE_TIMER, TIM_OCPreload_Enable); // required for PWM

  TIM_CtrlPWMOutputs(EXTMODULE_TIMER, ENABLE);
  TIM_DMACmd(EXTMODULE_TIMER, TIM_DMA_Update, ENABLE);

  // re-enable timer
  TIM_Cmd(EXTMODULE_TIMER, ENABLE);

  NVIC_EnableIRQ(EXTMODULE_TIMER_DMA_STREAM_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_DMA_STREAM_IRQn, 7);
}
#endif

#if defined(PXX1) && defined(EXTMODULE_USART)
void extmodulePxx1SerialStart()
{
  extmoduleInvertedSerialStart(EXTMODULE_PXX1_SERIAL_BAUDRATE);
}
#endif

void extmoduleSendNextFrame()
{
  switch (moduleState[EXTERNAL_MODULE].protocol) {
    case PROTOCOL_CHANNELS_PPM:
      EXTMODULE_TIMER->CCR1 = GET_MODULE_PPM_DELAY(EXTERNAL_MODULE) * 2;
      EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE | (GET_MODULE_PPM_POLARITY(EXTERNAL_MODULE) ? EXTMODULE_TIMER_OUTPUT_POLARITY : 0); //     // we are using complementary output so logic has to be reversed here
      EXTMODULE_TIMER->CCR2 = *(extmodulePulsesData.ppm.ptr - 1) - 4000; // 2mS in advance
      EXTMODULE_TIMER_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
      EXTMODULE_TIMER_DMA_STREAM->CR |= EXTMODULE_TIMER_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
      EXTMODULE_TIMER_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
      EXTMODULE_TIMER_DMA_STREAM->M0AR = CONVERT_PTR_UINT(extmodulePulsesData.ppm.pulses);
      EXTMODULE_TIMER_DMA_STREAM->NDTR = extmodulePulsesData.ppm.ptr - extmodulePulsesData.ppm.pulses;
      EXTMODULE_TIMER_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
      break;

#if defined(PXX1)
    case PROTOCOL_CHANNELS_PXX1_PULSES: {

      if (EXTMODULE_TIMER_DMA_STREAM->CR & DMA_SxCR_EN)
        return;

      // disable timer & DMA
      DMA_DeInit(EXTMODULE_TIMER_DMA_STREAM);
      TIM_Cmd(EXTMODULE_TIMER, DISABLE);

      DMA_InitTypeDef DMA_InitStructure;
      DMA_InitStructure.DMA_Channel = EXTMODULE_TIMER_DMA_CHANNEL;

      DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
      DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
      DMA_InitStructure.DMA_Memory0BaseAddr =
        CONVERT_PTR_UINT(extmodulePulsesData.pxx.getData());
      DMA_InitStructure.DMA_BufferSize = extmodulePulsesData.pxx.getSize();

      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
      DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      
      DMA_Init(EXTMODULE_TIMER_DMA_STREAM, &DMA_InitStructure);

      // start DMA request
      DMA_ITConfig(EXTMODULE_TIMER_DMA_STREAM, DMA_IT_TC, ENABLE);
      DMA_Cmd(EXTMODULE_TIMER_DMA_STREAM, ENABLE);

      // re-init timer
      EXTMODULE_TIMER->EGR = TIM_PSCReloadMode_Immediate;
      TIM_Cmd(EXTMODULE_TIMER, ENABLE);
      break;
    }
#endif

#if defined(PXX1) && defined(HARDWARE_EXTERNAL_MODULE_SIZE_SML)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
      extmoduleSendBuffer(extmodulePulsesData.pxx_uart.getData(), extmodulePulsesData.pxx_uart.getSize());
      break;
#endif

#if defined(PXX2)
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
    case PROTOCOL_CHANNELS_PXX2_LOWSPEED:
      extmoduleSendBuffer(extmodulePulsesData.pxx2.getData(), extmodulePulsesData.pxx2.getSize());
      break;
#endif
#if defined(AFHDS3)
    case PROTOCOL_CHANNELS_AFHDS3:
    {
#if defined(EXTMODULE_USART) && defined(EXTMODULE_TX_INVERT_GPIO)
      extmoduleSendBuffer(extmodulePulsesData.afhds3.getData(), extmodulePulsesData.afhds3.getSize());
#else
      const uint16_t* dataPtr = extmodulePulsesData.afhds3.getData();
      uint32_t dataSize = extmodulePulsesData.afhds3.getSize();
      EXTMODULE_TIMER->CCR2 = dataPtr[dataSize -1] - 4000; // 2mS in advance
      EXTMODULE_TIMER_DMA_STREAM->CR &= ~DMA_SxCR_EN; // Disable DMA
      EXTMODULE_TIMER_DMA_STREAM->CR |= EXTMODULE_TIMER_DMA_CHANNEL | DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PSIZE_0 | DMA_SxCR_MSIZE_0 | DMA_SxCR_PL_0 | DMA_SxCR_PL_1;
      EXTMODULE_TIMER_DMA_STREAM->PAR = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
      EXTMODULE_TIMER_DMA_STREAM->M0AR = CONVERT_PTR_UINT(dataPtr);
      EXTMODULE_TIMER_DMA_STREAM->NDTR = dataSize;
      EXTMODULE_TIMER_DMA_STREAM->CR |= DMA_SxCR_EN | DMA_SxCR_TCIE; // Enable DMA
#endif
    }
    break;
#endif
#if defined(SBUS) || defined(DSM2) || defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_SBUS:
      // no break
    case PROTOCOL_CHANNELS_DSM2_LP45:
    case PROTOCOL_CHANNELS_DSM2_DSM2:
    case PROTOCOL_CHANNELS_DSM2_DSMX:
    case PROTOCOL_CHANNELS_MULTIMODULE: {

      if (EXTMODULE_TIMER_DMA_STREAM->CR & DMA_SxCR_EN)
        return;

      // disable DMA & timer
      DMA_DeInit(EXTMODULE_TIMER_DMA_STREAM);
      TIM_Cmd(EXTMODULE_TIMER, DISABLE);

      if (PROTOCOL_CHANNELS_SBUS == moduleState[EXTERNAL_MODULE].protocol) {
        EXTMODULE_TIMER->CCER = EXTMODULE_TIMER_OUTPUT_ENABLE
          // reverse polarity for Sbus if needed:
          | (GET_SBUS_POLARITY(EXTERNAL_MODULE) ? EXTMODULE_TIMER_OUTPUT_POLARITY : 0);
      }

      // send DMA request
      DMA_InitTypeDef DMA_InitStructure;
      DMA_InitStructure.DMA_Channel = EXTMODULE_TIMER_DMA_CHANNEL;

      DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&EXTMODULE_TIMER->ARR);
      DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;

      // start address
      DMA_InitStructure.DMA_Memory0BaseAddr =
        CONVERT_PTR_UINT(extmodulePulsesData.dsm2.pulses);

      // transfer size
      DMA_InitStructure.DMA_BufferSize =
        extmodulePulsesData.dsm2.ptr - extmodulePulsesData.dsm2.pulses;

      DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
      DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
      DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
      DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
      DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
      DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
      DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
      DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
      DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      
      DMA_Init(EXTMODULE_TIMER_DMA_STREAM, &DMA_InitStructure);

      // start DMA request
      DMA_ITConfig(EXTMODULE_TIMER_DMA_STREAM, DMA_IT_TC, ENABLE);
      DMA_Cmd(EXTMODULE_TIMER_DMA_STREAM, ENABLE);

      // re-init timer
      EXTMODULE_TIMER->EGR = TIM_PSCReloadMode_Immediate;
      TIM_Cmd(EXTMODULE_TIMER, ENABLE);
      break;
    }
#endif

#if defined(CROSSFIRE)
    case PROTOCOL_CHANNELS_CROSSFIRE:
      sportSendBuffer(extmodulePulsesData.crossfire.pulses, extmodulePulsesData.crossfire.length);
      break;
#endif

#if defined(GHOST)
    case PROTOCOL_CHANNELS_GHOST:
      sportSendBuffer(extmodulePulsesData.ghost.pulses, extmodulePulsesData.ghost.length);
      break;
#endif

    default:
      EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;
      break;
  }
}

void extmoduleSendInvertedByte(uint8_t byte)
{
  uint16_t time;
  uint32_t i;

  __disable_irq();
  time = getTmr2MHz();
  GPIO_SetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
  while ((uint16_t) (getTmr2MHz() - time) < 34)	{
    // wait
  }
  time += 34;
  for (i = 0 ; i < 8 ; i += 1) {
    if (byte & 1) {
      GPIO_ResetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    }
    else {
      GPIO_SetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    }
    byte >>= 1 ;
    while ((uint16_t) (getTmr2MHz() - time) < 35) {
      // wait
    }
    time += 35 ;
  }
  GPIO_ResetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
  __enable_irq() ;	// No need to wait for the stop bit to complete
  while ((uint16_t) (getTmr2MHz() - time) < 34) {
    // wait
  }
}

extern "C" void EXTMODULE_TIMER_DMA_STREAM_IRQHandler()
{
  if (!DMA_GetITStatus(EXTMODULE_TIMER_DMA_STREAM, EXTMODULE_TIMER_DMA_FLAG_TC))
    return;

  DMA_ClearITPendingBit(EXTMODULE_TIMER_DMA_STREAM, EXTMODULE_TIMER_DMA_FLAG_TC);

  switch (moduleState[EXTERNAL_MODULE].protocol) {
  case PROTOCOL_CHANNELS_PPM:
    EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF; // Clear flag
    EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE; // Enable this interrupt
    break;
  }
}

extern "C" void EXTMODULE_TIMER_CC_IRQHandler()
{
  EXTMODULE_TIMER->DIER &= ~TIM_DIER_CC2IE; // Stop this interrupt
  EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;

  if (setupPulsesExternalModule()) {
    extmoduleSendNextFrame();
  }
}
