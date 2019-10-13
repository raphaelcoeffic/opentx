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

#ifndef _BUZZER_H_
#define _BUZZER_H_

inline void beep(uint8_t) { }

#if !defined(AUDIO)
  #define AUDIO_HELLO()
  #define AUDIO_BYE()
  #define AUDIO_TX_BATTERY_LOW()
  #define AUDIO_INACTIVITY()
  #define AUDIO_ERROR_MESSAGE(e)
  #define AUDIO_TIMER_MINUTE(t)
  #define AUDIO_TIMER_30()
  #define AUDIO_TIMER_20()
  #define AUDIO_WARNING2()
  #define AUDIO_WARNING1()
  #define AUDIO_ERROR()
  #define AUDIO_MIX_WARNING(x)
  #define AUDIO_POT_MIDDLE()
  #define AUDIO_TIMER_LT10(m, x)
  #define AUDIO_TIMER_00(m)
  #define AUDIO_VARIO_UP()
  #define AUDIO_VARIO_DOWN()
  #define AUDIO_TRIM(event, f)
  #define AUDIO_TRIM_MIDDLE(f)
  #define AUDIO_TRIM_END(f)
  #define AUDIO_PLAY(p)
  #define IS_AUDIO_BUSY() false

  #define AUDIO_RESET()
  #define AUDIO_FLUSH()

  #define PLAY_PHASE_OFF(phase)
  #define PLAY_PHASE_ON(phase)
  #define PLAY_SWITCH_MOVED(sw)
  #define PLAY_LOGICAL_SWITCH_OFF(sw)
  #define PLAY_LOGICAL_SWITCH_ON(sw)
  #define PLAY_MODEL_NAME()
  #define START_SILENCE_PERIOD()
#endif /* !AUDIO */
#endif // _BUZZER_H_
