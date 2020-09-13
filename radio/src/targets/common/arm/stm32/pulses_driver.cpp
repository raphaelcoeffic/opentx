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
#include "pulses_driver.h"

#define PULSE_INVALID_AF -1

static int8_t pulsesGetTimerAF(TIM_TypeDef* timer)
{
  if ((timer == TIM1)
      || (timer == TIM2)) {

    return GPIO_AF_TIM1;
  }
  else if ((timer == TIM3)
           || (timer == TIM4)
           || (timer == TIM5)) {

    return GPIO_AF_TIM3;
  }
  else if ((timer == TIM8)
           || (timer == TIM9)
           || (timer == TIM10)
           || (timer == TIM11)) {

    return GPIO_AF_TIM8;
  }

  return PULSE_INVALID_AF;
}

static void pulsesTimerInitGPIO(const PulsesTimerConfig& timerConfig)
{
  int8_t alt_func = pulsesGetTimerAF(timerConfig.timer);
  if (alt_func != PULSE_INVALID_AF) {
    GPIO_PinAFConfig(timerConfig.gpio, timerConfig.pinSource, alt_func);
  }

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin   = 1 << timerConfig.pinSource;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(timerConfig.gpio, &GPIO_InitStructure);
}

static void pulsesTimerInitTimeBase(const PulsesTimerConfig& timerConfig)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
  TIM_TimeBaseStruct.TIM_Prescaler         = timerConfig.prescaler;
  TIM_TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up;
  TIM_TimeBaseStruct.TIM_Period            = 0;
  TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
  TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(timerConfig.timer, &TIM_TimeBaseStruct);
}

typedef void (*TIM_OCxInit)(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
typedef void (*TIM_OCxPreloadConfig)(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload);

struct TimerChannelDriver {
  TIM_OCxInit          OCxInit;
  TIM_OCxPreloadConfig OCxPreloadConfig;
};

const TimerChannelDriver timerChannelDriver[4] = {
  { TIM_OC1Init, TIM_OC1PreloadConfig },
  { TIM_OC2Init, TIM_OC2PreloadConfig },
  { TIM_OC3Init, TIM_OC3PreloadConfig },
  { TIM_OC4Init, TIM_OC4PreloadConfig }
};

static void pulsesTimerInitOC(const PulsesTimerConfig& timerConfig, uint16_t polarity)
{
  TIM_OCInitTypeDef TIM_OCInitStruct;
  TIM_OCStructInit(&TIM_OCInitStruct);

  TIM_OCInitStruct.TIM_Pulse  = timerConfig.pulse;
  TIM_OCInitStruct.TIM_OCMode = TIM_ForcedAction_InActive;

  uint8_t ch_idx = timerConfig.channel >> 2; // channels: 0x00, 0x04, 0x08, 0x0C
  if (IS_TIM_CHANNEL(timerConfig.channel)) {
    timerChannelDriver[ch_idx].OCxInit(timerConfig.timer, &TIM_OCInitStruct);
  }

  timerConfig.timer->CCER = polarity;
  TIM_SelectOCxM(timerConfig.timer, timerConfig.channel, timerConfig.outputMode);

  if ((timerConfig.outputMode == TIM_OCMode_PWM1)
      && IS_TIM_CHANNEL(timerConfig.channel)) {

    // TIM_OCPreload_Enable is required for PWM mode
    timerChannelDriver[ch_idx].OCxPreloadConfig(timerConfig.timer, TIM_OCPreload_Enable);
  }

  TIM_CtrlPWMOutputs(timerConfig.timer, ENABLE);
  TIM_DMACmd(timerConfig.timer, TIM_DMA_Update, ENABLE);  
}

void pulsesTimerConfig(const PulsesTimerConfig& timerConfig, uint16_t polarity)
{
  // config pin
  pulsesTimerInitGPIO(timerConfig);

  // disable timer
  TIM_Cmd(timerConfig.timer, DISABLE);

  pulsesTimerInitTimeBase(timerConfig);
  pulsesTimerInitOC(timerConfig, polarity);

  // timer will be started once a DMA transfer is started
  //TIM_Cmd(timerConfig.timer, ENABLE);

  NVIC_EnableIRQ(timerConfig.dmaStreamIRQn);
  NVIC_SetPriority(timerConfig.dmaStreamIRQn, PULSES_TIMER_DMA_IRQ_PRIO);
}

void pulsesTimerSendFrame(const PulsesTimerConfig& timerConfig, uint16_t polarity,
                          const uint16_t* frameData, uint32_t frameBytes)
{
  if (timerConfig.dmaStream->CR & DMA_SxCR_EN)
    return;

  // disable DMA & timer
  DMA_DeInit(timerConfig.dmaStream);
  TIM_Cmd(timerConfig.timer, DISABLE);

  timerConfig.timer->CCER = polarity;

  // send DMA request
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_Channel = timerConfig.dmaChannel;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&timerConfig.timer->ARR);
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;

  // start address
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)(frameData);

  // transfer size
  DMA_InitStructure.DMA_BufferSize = frameBytes;

  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
      
  DMA_Init(timerConfig.dmaStream, &DMA_InitStructure);

  // start DMA request
  DMA_ITConfig(timerConfig.dmaStream, DMA_IT_TC, ENABLE);
  DMA_Cmd(timerConfig.dmaStream, ENABLE);

  // re-init timer
  timerConfig.timer->EGR = TIM_PSCReloadMode_Immediate;
  TIM_Cmd(timerConfig.timer, ENABLE);
}
