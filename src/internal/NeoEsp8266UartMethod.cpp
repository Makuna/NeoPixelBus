/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266 UART hardware

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)

-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.

NeoPixelBus is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixelBus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#ifdef ARDUINO_ARCH_ESP8266
#include "NeoEsp8266UartMethod.h"
#include <utility>
extern "C"
{
    #include <ets_sys.h>
}

const volatile uint8_t* ICACHE_RAM_ATTR NeoEsp8266UartContext::FillUartFifo(uint8_t uartNum,
    const volatile uint8_t* pixels,
    const volatile uint8_t* end)
{
    // Remember: UARTs send less significant bit (LSB) first so
    //      pushing ABCDEF byte will generate a 0FEDCBA1 signal,
    //      including a LOW(0) start & a HIGH(1) stop bits.
    // Also, we have configured UART to invert logic levels, so:
    const uint8_t _uartData[4] = {
        0b110111, // On wire: 1 000 100 0 [Neopixel reads 00]
        0b000111, // On wire: 1 000 111 0 [Neopixel reads 01]
        0b110100, // On wire: 1 110 100 0 [Neopixel reads 10]
        0b000100, // On wire: 1 110 111 0 [NeoPixel reads 11]
    };
    uint8_t avail = (UART_TX_FIFO_SIZE - GetTxFifoLength(uartNum)) / 4;
    if (end - pixels > avail)
    {
        end = pixels + avail;
    }
    while (pixels < end)
    {
        uint8_t subpix = *pixels++;
        Enqueue(uartNum, _uartData[(subpix >> 6) & 0x3]);
        Enqueue(uartNum, _uartData[(subpix >> 4) & 0x3]);
        Enqueue(uartNum, _uartData[(subpix >> 2) & 0x3]);
        Enqueue(uartNum, _uartData[subpix & 0x3]);
    }
    return pixels;
}

void NeoEsp8266UartInterruptContext::StartSending(uint8_t* start, uint8_t* end)
{
    // send the pixels asynchronously
    _asyncBuff = start;
    _asyncBuffEnd = end;

    // enable the transmit interrupt
    USIE(_uartNum) |= (1 << UIFE);
}

void NeoEsp8266UartInterruptContext::Attach()
{
    // Disable all interrupts
    ETS_UART_INTR_DISABLE();

    // Clear the RX & TX FIFOS
    const uint32_t fifoResetFlags = (1 << UCTXRST) | (1 << UCRXRST);
    USC0(_uartNum) |= fifoResetFlags;
    USC0(_uartNum) &= ~(fifoResetFlags);

    uart_subscribeInterrupt(_uartNum, Isr, this);

    // Set tx fifo trigger. 80 bytes gives us 200 microsecs to refill the FIFO
    USC1(_uartNum) = (80 << UCFET);

    // Disable RX & TX interrupts. It maybe still enabled by uart.c in the SDK
    USIE(_uartNum) &= ~((1 << UIFF) | (1 << UIFE));

    // Clear all pending interrupts in UART1
    USIC(_uartNum) = 0xffff;

    // Reenable interrupts
    ETS_UART_INTR_ENABLE();
}

void NeoEsp8266UartInterruptContext::Detach()
{
    // uart_unsubscribeInterrupt is safe and does INT enable and disable within it
    uart_unsubscribeInterrupt(_uartNum, Isr);
}

void ICACHE_RAM_ATTR NeoEsp8266UartInterruptContext::Isr(void* param)
{
    NeoEsp8266UartInterruptContext* that = static_cast<NeoEsp8266UartInterruptContext*>(param);

    // Fill the FIFO with new data
    that->_asyncBuff = FillUartFifo(
        that->_uartNum,
        that->_asyncBuff,
        that->_asyncBuffEnd);

    // Disable TX interrupt when done
    if (that->_asyncBuff == that->_asyncBuffEnd)
    {
        // clear the TX FIFO Empty
        USIE(that->_uartNum) &= ~(1 << UIFE);
    }
                
    // Clear all interrupts flags (just in case)
    USIC(that->_uartNum) = 0xffff;
}

#endif

