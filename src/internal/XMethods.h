/*-------------------------------------------------------------------------
NeoXMethods defines all the generic abstraction types that describe pulse/data sending methods 
for parallel methods

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by donating (see https://github.com/Makuna/NeoPixelBus)

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


#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)
//----------------------------------------------------------

#if defined(CONFIG_IDF_TARGET_ESP32S3)
//----------------------------------------------------------
#define NEO_X4_ALIAS

typedef NeoEsp32LcdX8Ws2812xMethod   X8Ws2812xMethod;
typedef NeoEsp32LcdX8Ws2805Method    X8Ws2805Method;
typedef NeoEsp32LcdX8Sk6812Method    X8Sk6812Method;
typedef NeoEsp32LcdX8Tm1814Method    X8Tm1814Method;
typedef NeoEsp32LcdX8Tm1829Method    X8Tm1829Method;
typedef NeoEsp32LcdX8Tm1914Method    X8Tm1914Method;
typedef NeoEsp32LcdX8800KbpsMethod   X8800KbpsMethod;
typedef NeoEsp32LcdX8400KbpsMethod   X8400KbpsMethod;
typedef NeoEsp32LcdX8Apa106Method    X8Apa106Method;
typedef NeoEsp32LcdX8Ws2814Method    X8Ws2814Method;
typedef NeoEsp32LcdX8Ws2813Method    X8Ws2813Method;
typedef NeoEsp32LcdX8Ws2812dMethod   X8Ws2812dMethod;
typedef NeoEsp32LcdX8Ws2811Method    X8Ws2811Method;
typedef NeoEsp32LcdX8Ws2816Method    X8Ws2816Method;
typedef NeoEsp32LcdX8Ws2812Method    X8Ws2812Method;
typedef NeoEsp32LcdX8Lc8812Method    X8Lc8812Method;

typedef NeoEsp32LcdX16Ws2812xMethod   X16Ws2812xMethod;
typedef NeoEsp32LcdX16Ws2805Method    X16Ws2805Method;
typedef NeoEsp32LcdX16Sk6812Method    X16Sk6812Method;
typedef NeoEsp32LcdX16Tm1814Method    X16Tm1814Method;
typedef NeoEsp32LcdX16Tm1829Method    X16Tm1829Method;
typedef NeoEsp32LcdX16Tm1914Method    X16Tm1914Method;
typedef NeoEsp32LcdX16800KbpsMethod   X16800KbpsMethod;
typedef NeoEsp32LcdX16400KbpsMethod   X16400KbpsMethod;
typedef NeoEsp32LcdX16Apa106Method    X16Apa106Method;
typedef NeoEsp32LcdX16Ws2814Method    X16Ws2814Method;
typedef NeoEsp32LcdX16Ws2813Method    X16Ws2813Method;
typedef NeoEsp32LcdX16Ws2812dMethod   X16Ws2812dMethod;
typedef NeoEsp32LcdX16Ws2811Method    X16Ws2811Method;
typedef NeoEsp32LcdX16Ws2816Method    X16Ws2816Method;
typedef NeoEsp32LcdX16Ws2812Method    X16Ws2812Method;
typedef NeoEsp32LcdX16Lc8812Method    X16Lc8812Method;

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
//----------------------------------------------------------
/* RMT doesnt have a X method yet
typedef NeoEsp32Rmtx8Ws2812xMethod   X8Ws2812xMethod;
typedef NeoEsp32Rmtx8Ws2805Method    X8Ws2805Method;
typedef NeoEsp32Rmtx8Sk6812Method    X8Sk6812Method;
typedef NeoEsp32Rmtx8Tm1814Method    X8Tm1814Method;
typedef NeoEsp32Rmtx8Tm1829Method    X8Tm1829Method;
typedef NeoEsp32Rmtx8Tm1914Method    X8Tm1914Method;
typedef NeoEsp32Rmtx8800KbpsMethod   X8800KbpsMethod;
typedef NeoEsp32Rmtx8400KbpsMethod   X8400KbpsMethod;
typedef NeoEsp32Rmtx8Apa106Method    X8Apa106Method;
typedef NeoEsp32Rmtx8Ws2814Method    X8Ws2814Method;
typedef NeoEsp32Rmtx8Ws2813Method    X8Ws2813Method;
typedef NeoEsp32Rmtx8Ws2812dMethod   X8Ws2812dMethod;
typedef NeoEsp32Rmtx8Ws2811Method    X8Ws2811Method;
typedef NeoEsp32Rmtx8Ws2816Method    X8Ws2816Method;
typedef NeoEsp32Rmtx8Ws2812Method    X8Ws2812Method;
typedef NeoEsp32Rmtx8Lc8812Method    X8Lc8812Method;
*/

#elif defined(CONFIG_IDF_TARGET_ESP32S2)
//----------------------------------------------------------
#define NEO_X4_ALIAS

typedef NeoEsp32I2s0X8Ws2812xMethod   X8Ws2812xMethod;
typedef NeoEsp32I2s0X8Ws2805Method    X8Ws2805Method;
typedef NeoEsp32I2s0X8Sk6812Method    X8Sk6812Method;
typedef NeoEsp32I2s0X8Tm1814Method    X8Tm1814Method;
typedef NeoEsp32I2s0X8Tm1829Method    X8Tm1829Method;
typedef NeoEsp32I2s0X8Tm1914Method    X8Tm1914Method;
typedef NeoEsp32I2s0X8800KbpsMethod   X8800KbpsMethod;
typedef NeoEsp32I2s0X8400KbpsMethod   X8400KbpsMethod;
typedef NeoEsp32I2s0X8Apa106Method    X8Apa106Method;
typedef NeoEsp32I2s0X8Ws2814Method    X8Ws2814Method;
typedef NeoEsp32I2s0X8Ws2813Method    X8Ws2813Method;
typedef NeoEsp32I2s0X8Ws2812dMethod   X8Ws2812dMethod;
typedef NeoEsp32I2s0X8Ws2811Method    X8Ws2811Method;
typedef NeoEsp32I2s0X8Ws2816Method    X8Ws2816Method;
typedef NeoEsp32I2s0X8Ws2812Method    X8Ws2812Method;
typedef NeoEsp32I2s0X8Lc8812Method    X8Lc8812Method;

typedef NeoEsp32I2s0X16Ws2812xMethod   X16Ws2812xMethod;
typedef NeoEsp32I2s0X16Ws2805Method    X16Ws2805Method;
typedef NeoEsp32I2s0X16Sk6812Method    X16Sk6812Method;
typedef NeoEsp32I2s0X16Tm1814Method    X16Tm1814Method;
typedef NeoEsp32I2s0X16Tm1829Method    X16Tm1829Method;
typedef NeoEsp32I2s0X16Tm1914Method    X16Tm1914Method;
typedef NeoEsp32I2s0X16800KbpsMethod   X16800KbpsMethod;
typedef NeoEsp32I2s0X16400KbpsMethod   X16400KbpsMethod;
typedef NeoEsp32I2s0X16Apa106Method    X16Apa106Method;
typedef NeoEsp32I2s0X16Ws2814Method    X16Ws2814Method;
typedef NeoEsp32I2s0X16Ws2813Method    X16Ws2813Method;
typedef NeoEsp32I2s0X16Ws2812dMethod   X16Ws2812dMethod;
typedef NeoEsp32I2s0X16Ws2811Method    X16Ws2811Method;
typedef NeoEsp32I2s0X16Ws2816Method    X16Ws2816Method;
typedef NeoEsp32I2s0X16Ws2812Method    X16Ws2812Method;
typedef NeoEsp32I2s0X16Lc8812Method    X16Lc8812Method;

#else // plain old ESP32
//----------------------------------------------------------
#define NEO_X4_ALIAS

typedef NeoEsp32I2s1X8Ws2812xMethod   X8Ws2812xMethod;
typedef NeoEsp32I2s1X8Ws2805Method    X8Ws2805Method;
typedef NeoEsp32I2s1X8Sk6812Method    X8Sk6812Method;
typedef NeoEsp32I2s1X8Tm1814Method    X8Tm1814Method;
typedef NeoEsp32I2s1X8Tm1829Method    X8Tm1829Method;
typedef NeoEsp32I2s1X8Tm1914Method    X8Tm1914Method;
typedef NeoEsp32I2s1X8800KbpsMethod   X8800KbpsMethod;
typedef NeoEsp32I2s1X8400KbpsMethod   X8400KbpsMethod;
typedef NeoEsp32I2s1X8Apa106Method    X8Apa106Method;
typedef NeoEsp32I2s1X8Ws2814Method    X8Ws2814Method;
typedef NeoEsp32I2s1X8Ws2813Method    X8Ws2813Method;
typedef NeoEsp32I2s1X8Ws2812dMethod   X8Ws2812dMethod;
typedef NeoEsp32I2s1X8Ws2811Method    X8Ws2811Method;
typedef NeoEsp32I2s1X8Ws2816Method    X8Ws2816Method;
typedef NeoEsp32I2s1X8Ws2812Method    X8Ws2812Method;
typedef NeoEsp32I2s1X8Lc8812Method    X8Lc8812Method;

typedef NeoEsp32I2s1X16Ws2812xMethod   X16Ws2812xMethod;
typedef NeoEsp32I2s1X16Ws2805Method    X16Ws2805Method;
typedef NeoEsp32I2s1X16Sk6812Method    X16Sk6812Method;
typedef NeoEsp32I2s1X16Tm1814Method    X16Tm1814Method;
typedef NeoEsp32I2s1X16Tm1829Method    X16Tm1829Method;
typedef NeoEsp32I2s1X16Tm1914Method    X16Tm1914Method;
typedef NeoEsp32I2s1X16800KbpsMethod   X16800KbpsMethod;
typedef NeoEsp32I2s1X16400KbpsMethod   X16400KbpsMethod;
typedef NeoEsp32I2s1X16Apa106Method    X16Apa106Method;
typedef NeoEsp32I2s1X16Ws2814Method    X16Ws2814Method;
typedef NeoEsp32I2s1X16Ws2813Method    X16Ws2813Method;
typedef NeoEsp32I2s1X16Ws2812dMethod   X16Ws2812dMethod;
typedef NeoEsp32I2s1X16Ws2811Method    X16Ws2811Method;
typedef NeoEsp32I2s1X16Ws2816Method    X16Ws2816Method;
typedef NeoEsp32I2s1X16Ws2812Method    X16Ws2812Method;
typedef NeoEsp32I2s1X16Lc8812Method    X16Lc8812Method;


#endif


#elif defined(ARDUINO_ARCH_RP2040) // must be before __arm__
//----------------------------------------------------------

typedef Rp2040x4Pio1Ws2812xMethod X4Ws2813Method;
typedef Rp2040x4Pio1800KbpsMethod X4Ws2812Method;
typedef Rp2040x4Pio1Ws2812xMethod X4Ws2811Method;
typedef Rp2040x4Pio1Ws2812xMethod X4Ws2816Method;
typedef Rp2040x4Pio1Ws2805Method  X4Ws2805Method;
typedef Rp2040x4Pio1Ws2814Method  X4Ws2814Method;
typedef Rp2040x4Pio1Sk6812Method  X4Sk6812Method;
typedef Rp2040x4Pio1Tm1814Method  X4Tm1814Method;
typedef Rp2040x4Pio1Tm1829Method  X4Tm1829Method;
typedef Rp2040x4Pio1Tm1914Method  X4Tm1914Method;
typedef Rp2040x4Pio1Sk6812Method  X4Lc8812Method;
typedef Rp2040x4Pio1Apa106Method  X4Apa106Method;
typedef Rp2040x4Pio1Tx1812Method  X4Tx1812Method;
typedef Rp2040x4Pio1Gs1903Method  X4Gs1903Method;

typedef Rp2040x4Pio1Ws2812xMethod X4800KbpsMethod;
typedef Rp2040x4Pio1Ws2812xMethod X4Ws2812xMethod;
typedef Rp2040x4Pio1400KbpsMethod X4400KbpsMethod;
#endif

// some plafforms do not have a native x4, so alias it to x8
//
#if defined(NEO_X4_ALIAS)
typedef X8Ws2812xMethod X4Ws2813Method;
typedef X8800KbpsMethod X4Ws2812Method;
typedef X8Ws2812xMethod X4Ws2811Method;
typedef X8Ws2812xMethod X4Ws2816Method;
typedef X8Ws2805Method  X4Ws2805Method;
typedef X8Ws2814Method  X4Ws2814Method;
typedef X8Sk6812Method  X4Sk6812Method;
typedef X8Tm1814Method  X4Tm1814Method;
typedef X8Tm1829Method  X4Tm1829Method;
typedef X8Tm1914Method  X4Tm1914Method;
typedef X8Sk6812Method  X4Lc8812Method;
typedef X8Apa106Method  X4Apa106Method;
//typedef X8Tx1812Method  X4Tx1812Method;
//typedef X8Gs1903Method  X4Gs1903Method;

typedef X8Ws2812xMethod X4800KbpsMethod;
typedef X8Ws2812xMethod X4Ws2812xMethod;
typedef X8400KbpsMethod X4400KbpsMethod;
#endif