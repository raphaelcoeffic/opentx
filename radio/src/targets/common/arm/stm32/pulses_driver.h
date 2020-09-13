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
#ifndef _PULSES_DRIVER_H_
#define _PULSES_DRIVER_H_

#include "board_common.h"

#define TIM_OCMODE_FORCED_ACTIVE (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_TOGGLE        (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)
#define TIM_OCMODE_PWM1          (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)

#define PULSES_TIMER_FREQ        2000000 // 2 MHz
#define PULSES_TIMER_PRESCALER(timer_freq) (timer_freq / PULSES_TIMER_FREQ - 1)

#define PULSES_TIMER_DMA_IRQ_PRIO 7

struct PulsesTimerConfig
{
    GPIO_TypeDef*       gpio;
    uint16_t            pinSource;

    TIM_TypeDef*        timer;
    uint16_t            channel;
    uint16_t            prescaler;
    uint16_t            outputMode; // OCMode
    uint16_t            pulse;

    DMA_Stream_TypeDef* dmaStream;
    IRQn_Type           dmaStreamIRQn;
    uint32_t            dmaChannel;
};

void pulsesTimerConfig(const PulsesTimerConfig& timerConfig, uint16_t polarity);

void pulsesTimerSendFrame(const PulsesTimerConfig& timerConfig, uint16_t polarity,
                          const uint16_t* frameData, uint32_t frameBytes);

#endif
