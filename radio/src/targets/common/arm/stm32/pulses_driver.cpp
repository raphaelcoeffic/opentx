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
  TIM_TimeBaseStruct.TIM_Period            = 0; // 40000; // TODO: fake value
  TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
  TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(timerConfig.timer, &TIM_TimeBaseStruct);
}

static void pulsesTimerInitOC(const PulsesTimerConfig& timerConfig, uint16_t polarity)
{
  TIM_OCInitTypeDef TIM_OCInitStruct;
  TIM_OCStructInit(&TIM_OCInitStruct);

  TIM_OCInitStruct.TIM_Pulse = 0; // whatever, will be replaced by DMA data...
  TIM_OCInitStruct.TIM_OCMode = TIM_ForcedAction_InActive; //Active;

  switch (timerConfig.channel){
  case TIM_Channel_1:
    TIM_OC1Init(timerConfig.timer, &TIM_OCInitStruct);
    break;
  case TIM_Channel_2:
    TIM_OC2Init(timerConfig.timer, &TIM_OCInitStruct);
    break;
  case TIM_Channel_3:
    TIM_OC3Init(timerConfig.timer, &TIM_OCInitStruct);
    break;
  case TIM_Channel_4:
    TIM_OC4Init(timerConfig.timer, &TIM_OCInitStruct);
    break;
  }

  timerConfig.timer->CCER = polarity;
  TIM_SelectOCxM(timerConfig.timer, timerConfig.channel, timerConfig.outputMode);

  TIM_CtrlPWMOutputs(timerConfig.timer, ENABLE);
  TIM_DMACmd(timerConfig.timer, TIM_DMA_Update, ENABLE);  
}

void pulsesTimerStart(const PulsesTimerConfig& timerConfig, uint16_t polarity)
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
