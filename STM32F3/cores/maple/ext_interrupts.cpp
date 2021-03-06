/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
 * Copyright (c) 2011, 2012 LeafLabs, LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

/**
 * @file ext_interrupts.cpp
 * @brief Wiring-like interface for external interrupts
 */

#include <ext_interrupts.h>

#include <libmaple/gpio.h>
#include <libmaple/exti.h>

#include <boards.h>

static inline exti_trigger_mode exti_out_mode(ExtIntTriggerMode mode);

void attachInterrupt(uint8 pin, voidFuncPtr handler, ExtIntTriggerMode mode) {
    if (pin >= LAST_GPIO_PIN || !handler) {
        return;
    }

    exti_trigger_mode outMode = exti_out_mode(mode);

    exti_attach_interrupt((exti_num)digitalPinToBit(pin),
                          (exti_cfg)(pin>>4),
                          handler,
                          outMode);
}

void attachInterrupt(uint8 pin, voidArgumentFuncPtr handler, void *arg,
                     ExtIntTriggerMode mode) {
    if (pin >= LAST_GPIO_PIN || !handler) {
        return;
    }

    exti_trigger_mode outMode = exti_out_mode(mode);

    exti_attach_callback((exti_num)digitalPinToBit(pin),
                          (exti_cfg)(pin>>4),
                          handler,
                          arg,
                          outMode);
}

void detachInterrupt(uint8 pin) {
    if (pin >= LAST_GPIO_PIN) {
        return;
    }

    exti_detach_interrupt((exti_num)digitalPinToBit(pin));
}

static inline exti_trigger_mode exti_out_mode(ExtIntTriggerMode mode) {
    switch (mode) {
    case RISING:
        return EXTI_RISING;
    case FALLING:
        return EXTI_FALLING;
    case CHANGE:
        return EXTI_RISING_FALLING;
    }
    // Can't happen
    ASSERT(0);
    return (exti_trigger_mode)0;
}
