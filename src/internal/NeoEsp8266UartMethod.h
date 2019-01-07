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

#pragma once

#ifdef ARDUINO_ARCH_ESP8266
#include <Arduino.h>

// this template method class is used to track the data being sent on the uart
// when using the default serial ISR installed by the core
// used with NeoEsp8266Uart and NeoEsp8266AsyncUart classes
//
class NeoEsp8266UartContext
{
public:
    // Gets the number of bytes waiting in the TX FIFO
    static inline uint8_t ICACHE_RAM_ATTR GetTxFifoLength(uint8_t uartNum)
    {
        return (USS(uartNum) >> USTXC) & 0xff;
    }
    // Append a byte to the TX FIFO
    static inline void ICACHE_RAM_ATTR Enqueue(uint8_t uartNum, uint8_t value)
    {
        USF(uartNum) = value;
    }

    static const volatile uint8_t* ICACHE_RAM_ATTR FillUartFifo(uint8_t uartNum,
        const volatile uint8_t* pixels,
        const volatile uint8_t* end);

    static inline void ConfigUart(uint8_t uartNum, uint32_t baud)
    {
        // set buad rate
        USD(uartNum) = (ESP8266_CLOCK / baud);

        // Clear the RX & TX FIFOS
        const uint32_t fifoResetFlags = (1 << UCTXRST) | (1 << UCRXRST);
        USC0(uartNum) |= fifoResetFlags;
        USC0(uartNum) &= ~(fifoResetFlags);

        // set config:
        // start with UART_6N1 which has all invert bits cleared
        // invert the TX voltage associated with logic level so:
        //    - A logic level 0 will generate a Vcc signal
        //    - A logic level 1 will generate a Gnd signal
        USC0(uartNum) = UART_6N1 | (1 << UCTXI);
        USC1(uartNum) = 0;
        USIC(uartNum) = 0xffff;
        USIE(uartNum) = 0;
    }
};

// this template method class is used to track the data being sent on the uart
// when using our own UART ISR 
// used with NeoEsp8266Uart and NeoEsp8266AsyncUart classes
//
class NeoEsp8266UartInterruptContext : public NeoEsp8266UartContext
{
public:
    NeoEsp8266UartInterruptContext(uint8_t index) :
        _uartNum(index),
        _asyncBuff(nullptr),
        _asyncBuffEnd(nullptr)
    {
    }

    bool IsSending()
    {
        return (_asyncBuff != _asyncBuffEnd);
    }

    void Attach();
    void Detach();

    void StartSending(uint8_t* start, uint8_t* end);

private:
    const uint8_t _uartNum;
    volatile const uint8_t* _asyncBuff;
    volatile const uint8_t* _asyncBuffEnd;

    static void ICACHE_RAM_ATTR Isr(void* param);
};

// this template feature class is used to define the specifics for uart0
// used with NeoEsp8266Uart and NeoEsp8266AsyncUart classes
//
class Uart0Feature 
{
public:
    static const uint32_t Index = 0;
    static void Init()
    {
        pinMode(1, FUNCTION_0);
    }
};

// this template feature class is used to define the specifics for uart1
// used with NeoEsp8266Uart and NeoEsp8266AsyncUart classes
//
class Uart1Feature
{
public:
    static const uint32_t Index = 1;
    static void Init()
    {
        pinMode(2, SPECIAL);
    }
};

// this template method class is used a base for all others and contains 
// common properties and methods
//
// used by NeoEsp8266Uart and NeoEsp8266AsyncUart
//
class NeoEsp8266UartBase
{
protected:
    size_t    _sizePixels;   // Size of '_pixels' buffer below
    uint8_t* _pixels;        // Holds LED color values
    uint32_t _startTime;     // Microsecond count when last update started

    NeoEsp8266UartBase(uint16_t pixelCount, size_t elementSize)
    {
        _sizePixels = pixelCount * elementSize;
        _pixels = (uint8_t*)malloc(_sizePixels);
        memset(_pixels, 0x00, _sizePixels);
    }

    ~NeoEsp8266UartBase()
    {
        free(_pixels);
    }
};

// this template method class is used to glue uart feature and context for
// synchronous uart method
//
// used by NeoEsp8266UartMethodBase
//
template<typename T_UARTFEATURE, typename T_UARTCONTEXT> class NeoEsp8266Uart : public NeoEsp8266UartBase
{
protected:

    NeoEsp8266Uart(uint16_t pixelCount, size_t elementSize) :
        NeoEsp8266UartBase(pixelCount, elementSize)
    {
    }

    ~NeoEsp8266Uart()
    {
        // Wait until the TX fifo is empty. This way we avoid broken frames
        // when destroying & creating a NeoPixelBus to change its length.
        while (T_UARTCONTEXT::GetTxFifoLength(T_UARTFEATURE::Index) > 0)
        {
            yield();
        }
    }

    void InitializeUart(uint32_t uartBaud)
    {
        T_UARTFEATURE::Init();
        T_UARTCONTEXT::ConfigUart(T_UARTFEATURE::Index, uartBaud);
    }

    void UpdateUart()
    {
        // Since the UART can finish sending queued bytes in the FIFO in
        // the background, instead of waiting for the FIFO to flush
        // we annotate the start time of the frame so we can calculate
        // when it will finish.
        _startTime = micros();

        // Then keep filling the FIFO until done
        const uint8_t* ptr = _pixels;
        const uint8_t* end = ptr + _sizePixels;
        while (ptr != end)
        {
            ptr = const_cast<uint8_t*>(T_UARTCONTEXT::FillUartFifo(T_UARTFEATURE::Index, ptr, end));
        }
    }

};

// this template method class is used to glue uart feature and context for
// asynchronously uart method
//
// This UART controller uses two buffers that are swapped in every call to
// NeoPixelBus.Show(). One buffer contains the data that is being sent
// asynchronosly and another buffer contains the data that will be send
// in the next call to NeoPixelBus.Show().
//
// Therefore, the result of NeoPixelBus.Pixels() is invalidated after
// every call to NeoPixelBus.Show() and must not be cached.
//
// used by NeoEsp8266UartMethodBase
//
template<typename T_UARTFEATURE, typename T_UARTCONTEXT> class NeoEsp8266AsyncUart : public NeoEsp8266UartBase
{
protected:
    NeoEsp8266AsyncUart(uint16_t pixelCount, size_t elementSize) :
        NeoEsp8266UartBase(pixelCount, elementSize),
        _context(T_UARTFEATURE::Index)
    {
        _asyncPixels = (uint8_t*)malloc(_sizePixels);
    }

    ~NeoEsp8266AsyncUart()
    {
        // Remember: the UART interrupt can be sending data from _asyncPixels in the background
        while (_context.IsSending())
        {
            yield();
        }
        // detach context, which will disable intr, may disable ISR
        _context.Detach();
        
        free(_asyncPixels);
    }

    void ICACHE_RAM_ATTR InitializeUart(uint32_t uartBaud)
    {
        T_UARTFEATURE::Init();
        T_UARTCONTEXT::ConfigUart(T_UARTFEATURE::Index, uartBaud);

        // attach the context, which will enable the ISR
        _context.Attach();
    }

    void UpdateUart()
    {
        // Instruct ESP8266 hardware uart to send the pixels asynchronously
        _context.StartSending(_pixels, _pixels + _sizePixels);

        // Annotate when we started to send bytes, so we can calculate when we are ready to send again
        _startTime = micros();

        // Copy the pixels to the idle buffer and swap them
        memcpy(_asyncPixels, _pixels, _sizePixels);
        std::swap(_asyncPixels, _pixels);
    }

private:
    T_UARTCONTEXT _context;

    uint8_t* _asyncPixels;  // Holds a copy of LED color values taken when UpdateUart began
};

class NeoEsp8266UartSpeed800KbpsBase
{
public:
    static const uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
    static const uint32_t UartBaud = 3200000; // 800mhz, 4 serial bytes per NeoByte
};

// NeoEsp8266UartSpeedWs2813 contains the timing constants used to get NeoPixelBus running with the Ws2813
class NeoEsp8266UartSpeedWs2812x : public NeoEsp8266UartSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 300; // us between data send bursts to reset for next update
};

class NeoEsp8266UartSpeedSk6812 : public NeoEsp8266UartSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 80; // us between data send bursts to reset for next update
};

// NeoEsp8266UartSpeed800Kbps contains the timing constant used to get NeoPixelBus running at 800Khz
class NeoEsp8266UartSpeed800Kbps : public NeoEsp8266UartSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 50; // us between data send bursts to reset for next update
};

// NeoEsp8266UartSpeed400Kbps contains the timing constant used to get NeoPixelBus running at 400Khz
class NeoEsp8266UartSpeed400Kbps
{
public:
    static const uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    static const uint32_t UartBaud = 1600000; // 400mhz, 4 serial bytes per NeoByte
    static const uint32_t ResetTimeUs = 50; // us between data send bursts to reset for next update
};

// NeoEsp8266UartMethodBase is a light shell arround NeoEsp8266Uart or NeoEsp8266AsyncUart that
// implements the methods needed to operate as a NeoPixelBus method.
template<typename T_SPEED, typename T_BASE>
class NeoEsp8266UartMethodBase: public T_BASE
{
public:
    NeoEsp8266UartMethodBase(uint16_t pixelCount, size_t elementSize)
        : T_BASE(pixelCount, elementSize)
    {
    }
    NeoEsp8266UartMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize)
        : T_BASE(pixelCount, elementSize)
    {
    }

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - this->_startTime;
        return delta >= getPixelTime() + T_SPEED::ResetTimeUs;
    }

    void Initialize()
    {
        this->InitializeUart(T_SPEED::UartBaud);

        // Inverting logic levels can generate a phantom bit in the led strip bus
        // We need to delay 50+ microseconds the output stream to force a data
        // latch and discard this bit. Otherwise, that bit would be prepended to
        // the first frame corrupting it.
        this->_startTime = micros() - getPixelTime();
    }

    void Update()
    {
        // Data latch = 50+ microsecond pause in the output stream.  Rather than
        // put a delay at the end of the function, the ending time is noted and
        // the function will simply hold off (if needed) on issuing the
        // subsequent round of data until the latch time has elapsed.  This
        // allows the mainline code to start generating the next frame of data
        // rather than stalling for the latch.
        while (!this->IsReadyToUpdate())
        {
            yield();
        }
        this->UpdateUart();
    }

    uint8_t* getPixels() const
    {
        return this->_pixels;
    };

    size_t getPixelsSize() const
    {
        return this->_sizePixels;
    };

private:
    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * this->_sizePixels);
    };
};

// uart 0 
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedWs2812x, NeoEsp8266Uart<Uart0Feature, NeoEsp8266UartContext>> NeoEsp8266Uart0Ws2812xMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedSk6812, NeoEsp8266Uart<Uart0Feature, NeoEsp8266UartContext>> NeoEsp8266Uart0Sk6812Method;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed800Kbps, NeoEsp8266Uart<Uart0Feature, NeoEsp8266UartContext>> NeoEsp8266Uart0800KbpsMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed400Kbps, NeoEsp8266Uart<Uart0Feature, NeoEsp8266UartContext>> NeoEsp8266Uart0400KbpsMethod;

typedef NeoEsp8266Uart0Ws2812xMethod NeoEsp8266Uart0Ws2813Method;
typedef NeoEsp8266Uart0800KbpsMethod NeoEsp8266Uart0Ws2812Method;
typedef NeoEsp8266Uart0Sk6812Method NeoEsp8266Uart0Lc8812Method;

// uart 1
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedWs2812x, NeoEsp8266Uart<Uart1Feature, NeoEsp8266UartContext>> NeoEsp8266Uart1Ws2812xMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedSk6812, NeoEsp8266Uart<Uart1Feature, NeoEsp8266UartContext>> NeoEsp8266Uart1Sk6812Method;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed800Kbps, NeoEsp8266Uart<Uart1Feature, NeoEsp8266UartContext>> NeoEsp8266Uart1800KbpsMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed400Kbps, NeoEsp8266Uart<Uart1Feature, NeoEsp8266UartContext>> NeoEsp8266Uart1400KbpsMethod;

typedef NeoEsp8266Uart1Ws2812xMethod NeoEsp8266Uart1Ws2813Method;
typedef NeoEsp8266Uart1800KbpsMethod NeoEsp8266Uart1Ws2812Method;
typedef NeoEsp8266Uart1Sk6812Method NeoEsp8266Uart1Lc8812Method;

// uart 0 async
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedWs2812x, NeoEsp8266AsyncUart<Uart0Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart0Ws2812xMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedSk6812, NeoEsp8266AsyncUart<Uart0Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart0Sk6812Method;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed800Kbps, NeoEsp8266AsyncUart<Uart0Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart0800KbpsMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed400Kbps, NeoEsp8266AsyncUart<Uart0Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart0400KbpsMethod;

typedef NeoEsp8266AsyncUart0Ws2812xMethod NeoEsp8266AsyncUart0Ws2813Method;
typedef NeoEsp8266AsyncUart0800KbpsMethod NeoEsp8266AsyncUart0Ws2812Method;
typedef NeoEsp8266AsyncUart0Sk6812Method NeoEsp8266AsyncUart0Lc8812Method;

// uart 1 async
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedWs2812x, NeoEsp8266AsyncUart<Uart1Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart1Ws2812xMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeedSk6812, NeoEsp8266AsyncUart<Uart1Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart1Sk6812Method;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed800Kbps, NeoEsp8266AsyncUart<Uart1Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart1800KbpsMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed400Kbps, NeoEsp8266AsyncUart<Uart1Feature, NeoEsp8266UartInterruptContext>> NeoEsp8266AsyncUart1400KbpsMethod;

typedef NeoEsp8266AsyncUart1Ws2812xMethod NeoEsp8266AsyncUart1Ws2813Method;
typedef NeoEsp8266AsyncUart1800KbpsMethod NeoEsp8266AsyncUart1Ws2812Method;
typedef NeoEsp8266AsyncUart1Sk6812Method NeoEsp8266AsyncUart1Lc8812Method;

#endif

