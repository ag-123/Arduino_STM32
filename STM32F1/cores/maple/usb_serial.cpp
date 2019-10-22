/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 Perry Hung.
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
 * @brief USB virtual serial terminal
 */

#include "usb_serial.h"

#include "string.h"
#include "stdint.h"

#include <libmaple/nvic.h>
#include <libmaple/usb/usb.h>
#include <libmaple/iwdg.h>
#include <libmaple/bkp.h>
#include "wirish.h"

/*
 * Hooks used for bootloader reset signalling
 */

#if BOARD_HAVE_SERIALUSB
static void rxHook(unsigned);
static void ifaceSetupHook(unsigned);
#endif

/*
 * USBSerial interface
 */

#define USB_TIMEOUT 50
#if BOARD_HAVE_SERIALUSB
bool USBSerial::_hasBegun = false;
bool USBSerial::_isBlocking = false;
#endif

USBSerial::USBSerial(void) {
#if !BOARD_HAVE_SERIALUSB
    ASSERT(0);
#endif
}

void USBSerial::begin(void)
{
#if BOARD_HAVE_SERIALUSB
    if (_hasBegun)
        return;
    _hasBegun = true;

    usb_cdcacm_enable();
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rxHook);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);
#endif
}

//Roger Clark. Two new begin functions has been added so that normal Arduino Sketches that use Serial.begin(xxx) will compile.
void USBSerial::begin(unsigned long ignoreBaud)
{
volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
	begin();
}
void USBSerial::begin(unsigned long ignoreBaud, uint8_t ignore)
{
volatile unsigned long removeCompilerWarningsIgnoreBaud=ignoreBaud;
volatile uint8_t removeCompilerWarningsIgnore=ignore;

	ignoreBaud=removeCompilerWarningsIgnoreBaud;
	ignore=removeCompilerWarningsIgnore;
	begin();
}

void USBSerial::end(void)
{
#if BOARD_HAVE_SERIALUSB
    usb_cdcacm_disable();
    usb_cdcacm_remove_hooks(USB_CDCACM_HOOK_RX | USB_CDCACM_HOOK_IFACE_SETUP);
	_hasBegun = false;
#endif
}

size_t USBSerial::write(uint8 ch) {
    return this->write(&ch, 1);
}

size_t USBSerial::write(const char *str) {
    return this->write((const uint8*)str, strlen(str));
}

size_t USBSerial::write(const uint8 *buf, uint32 len)
{
#ifdef USB_SERIAL_REQUIRE_DTR
    if (!(bool) *this || !buf) {
        return 0;
    }
#else	
	if (!buf || !(usb_is_connected(USBLIB) && usb_is_configured(USBLIB))) {
        return 0;
    }
#endif	

    uint32 txed = 0;
	if (!_isBlocking) 	{
		txed = usb_cdcacm_tx((const uint8*)buf + txed, len - txed);
	}
	else {
		while (txed < len) {
			txed += usb_cdcacm_tx((const uint8*)buf + txed, len - txed);
		}
	}

	return txed;
}

int USBSerial::peek(void)
{
    uint8 b;
	if (usb_cdcacm_peek(&b, 1)==1)
	{
		return b;
	}
	else
	{
		return -1;
	}
}

void USBSerial::flush(void)
{
/*Roger Clark. Rather slow method. Need to improve this */
    uint8 b;
	while(usb_cdcacm_data_available())
	{
		this->read(&b, 1);
	}
    return;
}

uint32 USBSerial::read(uint8 * buf, uint32 len) {
    uint32 rxed = 0;
    while (rxed < len) {
        rxed += usb_cdcacm_rx(buf + rxed, len - rxed);
    }

    return rxed;
}

size_t USBSerial::readBytes(char *buf, const size_t& len)
{
    size_t rxed=0;
    unsigned long startMillis;
    startMillis = millis();
    if (len <= 0) return 0;
    do {
        rxed += usb_cdcacm_rx((uint8 *)buf + rxed, len - rxed);
        if (rxed == len) return rxed;
    } while(millis() - startMillis < _timeout);
    return rxed;
}

/* Blocks forever until 1 byte is received */
int USBSerial::read(void) {
    uint8 b;
	if (usb_cdcacm_rx(&b, 1)==0)
	{
		return -1;
	}
	else
	{
		return b;
	}
}

uint8 USBSerial::pending(void) {
    return usb_cdcacm_get_pending();
}

uint8 USBSerial::getDTR(void) {
    return usb_cdcacm_get_dtr();
}

uint8 USBSerial::getRTS(void) {
    return usb_cdcacm_get_rts();
}

USBSerial::operator bool() {
    return usb_is_connected(USBLIB) && usb_is_configured(USBLIB) && usb_cdcacm_get_dtr();
}

void USBSerial::enableBlockingTx(void)
{
	_isBlocking=true;
}
void USBSerial::disableBlockingTx(void)
{
	_isBlocking=false;
}


#if BOARD_HAVE_SERIALUSB
	#ifdef SERIAL_USB 
		USBSerial Serial;
	#endif
#endif

/*
 * Bootloader hook implementations
 */

#if BOARD_HAVE_SERIALUSB

enum reset_state_t {
    DTR_UNSET,
    DTR_HIGH,
    DTR_NEGEDGE,
    DTR_LOW
};

static reset_state_t reset_state = DTR_UNSET;

static void ifaceSetupHook(unsigned requestvp)
{
    // Ignore requests we're not interested in.
    if (requestvp != USB_CDCACM_SET_CONTROL_LINE_STATE) {
        return;
    }

#ifdef SERIAL_USB 
    // We need to see a negative edge on DTR before we start looking
    // for the in-band magic reset byte sequence.
    uint8 dtr = usb_cdcacm_get_dtr();
    switch (reset_state) {
    case DTR_UNSET:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_HIGH:
        reset_state = dtr ? DTR_HIGH : DTR_NEGEDGE;
        break;
    case DTR_NEGEDGE:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    case DTR_LOW:
        reset_state = dtr ? DTR_HIGH : DTR_LOW;
        break;
    }
#endif

}

static const uint8 magic[4] = {'1', 'E', 'A', 'F'};	
static void rxHook(unsigned ignore)
{
	(void)ignore;
    /* FIXME this is mad buggy; we need a new reset sequence. E.g. NAK
     * after each RX means you can't reset if any bytes are waiting. */
    if (reset_state == DTR_NEGEDGE) {
        int len = usb_cdcacm_data_available();
        if (len >= 4) 
        {
            uint8 chkBuf[256];

            // Peek at the waiting bytes, looking for reset sequence,
            // bailing on mismatch.
            usb_cdcacm_peek(chkBuf, len);

            for (unsigned i = 0; i < sizeof(magic); i++) {
                if (chkBuf[len + i - 4] != magic[i]) 
                {
                    reset_state = DTR_LOW;
                    return;
                }
            }

#ifdef SERIAL_USB 
            // The magic reset sequence is "1EAF".
            // Got the magic sequence -> reset, presumably into the bootloader.
			bkp_init();
			bkp_enable_writes();
			bkp_write(10, 0x424C);
			bkp_disable_writes();
			nvic_sys_reset();			
#endif
            /* Can't happen. */
            ASSERT_FAULT(0);
        }
    }
}
#endif  // BOARD_HAVE_SERIALUSB