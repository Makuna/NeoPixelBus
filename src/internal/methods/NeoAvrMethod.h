/*-------------------------------------------------------------------------
NeoPixel library helper functions for Atmel AVR.

Written by Michael C. Miller.
Some work taken from the Adafruit NeoPixel library.

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

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)

extern "C"
{
    void send_data_8mhz_800_PortD(uint8_t* data, size_t sizeData, uint8_t pinMask);
    void send_data_8mhz_800_PortB(uint8_t* data, size_t sizeData, uint8_t pinMask);
    void send_data_8mhz_400(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask);
    void send_data_12mhz_800_PortD(uint8_t* data, size_t sizeData, uint8_t pinMask);
    void send_data_12mhz_800_PortB(uint8_t* data, size_t sizeData, uint8_t pinMask);
    void send_data_12mhz_400(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask);
    void send_data_16mhz_800(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask);
    void send_data_16mhz_400(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask);
    void send_data_16mhz_600(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask);
    void send_data_32mhz(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask, const uint8_t cycleTiming);
}

class NeoAvrSpeed800KbpsBase
{
public:
    static void send_data(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask)
    {
#if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)  // 8Mhz CPU
#ifdef PORTD // PORTD isn't present on ATtiny85, etc.
        if (port == &PORTD)
            send_data_8mhz_800_PortD(data, sizeData, pinMask);
        else if (port == &PORTB)
#endif // PORTD
            send_data_8mhz_800_PortB(data, sizeData, pinMask);

#elif (F_CPU >= 11100000UL) && (F_CPU <= 14300000UL)  // 12Mhz CPU
#ifdef PORTD // PORTD 
        if (port == &PORTD)
            send_data_12mhz_800_PortD(data, sizeData, pinMask);
        else if (port == &PORTB)
#endif // PORTD
            send_data_12mhz_800_PortB(data, sizeData, pinMask);

#elif (F_CPU >= 15400000UL) && (F_CPU <= 19000000UL)  // 16Mhz CPU
        send_data_16mhz_800(data, sizeData, port, pinMask);
#elif (F_CPU >= 31000000UL) && (F_CPU <= 35000000UL)  // 32Mhz CPU
        send_data_32mhz(data, sizeData, port, pinMask, 3);
#else
#error "CPU SPEED NOT SUPPORTED"
#endif
    }
    
};

class NeoAvrSpeed600KbpsBase
{
public:
    static void send_data(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask)
    {
#if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)  // 8Mhz CPU
#ifdef PORTD // PORTD isn't present on ATtiny85, etc.
        if (port == &PORTD)
            send_data_8mhz_800_PortD(data, sizeData, pinMask);
        else if (port == &PORTB)
#endif // PORTD
            send_data_8mhz_800_PortB(data, sizeData, pinMask);

#elif (F_CPU >= 11100000UL) && (F_CPU <= 14300000UL)  // 12Mhz CPU
#ifdef PORTD // PORTD 
        if (port == &PORTD)
            send_data_12mhz_800_PortD(data, sizeData, pinMask);
        else if (port == &PORTB)
#endif // PORTD
            send_data_12mhz_800_PortB(data, sizeData, pinMask);

#elif (F_CPU >= 15400000UL) && (F_CPU <= 19000000UL)  // 16Mhz CPU
        send_data_16mhz_600(data, sizeData, port, pinMask);
#elif (F_CPU >= 31000000UL) && (F_CPU <= 35000000UL)  // 32Mhz CPU
        send_data_32mhz(data, sizeData, port, pinMask, 3);
#else
#error "CPU SPEED NOT SUPPORTED"
#endif
    }

};

class NeoAvrSpeedWs2812x :  public NeoAvrSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoAvrSpeedSk6812 : public NeoAvrSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 80;
};

class NeoAvrSpeedApa106 : public NeoAvrSpeed600KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 100;
};

class NeoAvrSpeedTm1814 : public NeoAvrSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoAvrSpeedTm1829 : public NeoAvrSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoAvrSpeed800Kbps: public NeoAvrSpeed800KbpsBase
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoAvrSpeed400Kbps
{
public:
    static void send_data(uint8_t* data, size_t sizeData, volatile uint8_t* port, uint8_t pinMask)
    {
#if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)  // 8Mhz CPU
        send_data_8mhz_400(data, sizeData, port, pinMask);

#elif (F_CPU >= 11100000UL) && (F_CPU <= 14300000UL)  // 12Mhz CPU
        send_data_12mhz_400(data, sizeData, port, pinMask);

#elif (F_CPU >= 15400000UL) && (F_CPU <= 19000000UL)  // 16Mhz CPU
        send_data_16mhz_400(data, sizeData, port, pinMask);
#elif (F_CPU >= 31000000UL) && (F_CPU <= 35000000UL)  // 32Mhz CPU
        send_data_32mhz(data, sizeData, port, pinMask, 7);
#else
#error "CPU SPEED NOT SUPPORTED"
#endif
    }
    static const uint32_t ResetTimeUs = 50;
};

template<typename T_SPEED> class NeoAvrMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoAvrMethodBase(uint8_t pin, 
        uint16_t pixelCount, 
        [[maybe_unused]] size_t elementSize, 
        [[maybe_unused]] size_t settingsSize) :
        _pin(pin),
        _pixelCount(pixelCount)
    {
        pinMode(pin, OUTPUT);
        _port = portOutputRegister(digitalPinToPort(pin));
        _pinMask = digitalPinToBitMask(pin);
    }

    ~NeoAvrMethodBase()
    {
        pinMode(_pin, INPUT);
    }

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - _endTime;

        return (delta >= T_SPEED::ResetTimeUs);
    }

    void Initialize()
    {
        digitalWrite(_pin, LOW);

        _endTime = micros();
    }

    template <typename T_COLOR_OBJECT,
        typename T_COLOR_FEATURE,
        typename T_SHADER> 
    void Update(
        T_COLOR_OBJECT* pixels,
        size_t countPixels,
        const typename T_COLOR_FEATURE::SettingsObject& featureSettings,
        const T_SHADER& shader)
    {
        while (!IsReadyToUpdate())
        {
#if !defined(NEOPIXEBUS_NO_YIELD)
            yield(); // allows for system yield if needed
#endif
        }

        const size_t sendDataSize = T_COLOR_FEATURE::SettingsSize >= T_COLOR_FEATURE::PixelSize ? T_COLOR_FEATURE::SettingsSize : T_COLOR_FEATURE::PixelSize;
        uint8_t sendData[sendDataSize];

        noInterrupts(); // Need 100% focus on instruction timing

        // if there are settings at the front
        //
        if (T_COLOR_FEATURE::applyFrontSettings(sendData, sendDataSize, featureSettings))
        {
            T_SPEED::send_data(sendData, T_COLOR_FEATURE::SettingsSize, _port, _pinMask);
        }
        
        // send primary color data
        //
        T_COLOR_OBJECT* pixel = pixels;
        const T_COLOR_OBJECT* pixelEnd = pixel + countPixels;
        uint16_t stripCount = _pixelCount;

        while (stripCount--)
        {
            typename T_COLOR_FEATURE::ColorObject color = shader.Apply(*pixel);
            T_COLOR_FEATURE::applyPixelColor(sendData, sendDataSize, color);

            T_SPEED::send_data(sendData, T_COLOR_FEATURE::PixelSize, _port, _pinMask);

            pixel++;
            if (pixel >= pixelEnd)
            {
                // restart at first
                pixel = pixels;
            }
        }

        // if there are settings at the back
        //
        if (T_COLOR_FEATURE::applyBackSettings(sendData, sendDataSize, featureSettings))
        {
            T_SPEED::send_data(sendData, T_COLOR_FEATURE::SettingsSize, _port, _pinMask);
        }

        interrupts();

        // save EOD time for latch on next call
        _endTime = micros();
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

protected:
    const uint8_t _pin;         // output pin number
    const uint16_t _pixelCount; // count of pixels in the strip
    volatile uint8_t* _port;         // Output PORT register

    uint32_t _endTime;       // Latch timing reference
    uint8_t  _pinMask;      // Output PORT bitmask
};

typedef NeoAvrMethodBase<NeoAvrSpeedWs2812x> NeoAvrWs2812xMethod;
typedef NeoAvrMethodBase<NeoAvrSpeedSk6812> NeoAvrSk6812Method;
typedef NeoAvrMethodBase<NeoAvrSpeedApa106> NeoAvrApa106Method;

typedef NeoAvrMethodBase<NeoAvrSpeedTm1814> NeoAvrTm1814InvertedMethod;
typedef NeoAvrMethodBase<NeoAvrSpeedTm1829> NeoAvrTm1829InvertedMethod;
typedef NeoAvrMethodBase<NeoAvrSpeed800Kbps> NeoAvr800KbpsMethod;
typedef NeoAvrMethodBase<NeoAvrSpeed400Kbps> NeoAvr400KbpsMethod;
typedef NeoAvrTm1814InvertedMethod NeoAvrTm1914InvertedMethod;

// AVR doesn't have alternatives yet, so there is just the default
typedef NeoAvrWs2812xMethod NeoWs2805Method;
typedef NeoWs2805Method NeoWs2814Method;
typedef NeoAvrWs2812xMethod NeoWs2813Method;
typedef NeoAvrWs2812xMethod NeoWs2812xMethod;
typedef NeoAvr800KbpsMethod NeoWs2812Method;
typedef NeoAvrWs2812xMethod NeoWs2811Method;
typedef NeoAvrWs2812xMethod NeoWs2816Method;
typedef NeoAvrWs2812xMethod NeoWs2805Method;
typedef NeoWs2805Method NeoWs2814Method;
typedef NeoAvrSk6812Method NeoSk6812Method;
typedef NeoAvrSk6812Method NeoLc8812Method;
typedef NeoAvrApa106Method NeoApa106Method;
typedef NeoAvrWs2812xMethod Neo800KbpsMethod;
typedef NeoAvr400KbpsMethod Neo400KbpsMethod;

// there is no non-invert methods for avr, but the norm for TM1814 is inverted, so
typedef NeoAvrTm1814InvertedMethod NeoTm1814InvertedMethod;
typedef NeoAvrTm1914InvertedMethod NeoTm1914InvertedMethod;
typedef NeoAvrTm1829InvertedMethod NeoTm1829InvertedMethod;
#endif

