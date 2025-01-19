/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using general Pins (APA102).

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

/*
  Pixie reads data in at 115.2k serial, 8N1.
  Byte order is R1, G1, B1, R2, G2, B2, ... where the first triplet is the
  color of the LED that's closest to the controller. 1ms of silence triggers
  latch. 2 seconds silence (or overheating) triggers LED off (for safety).
*/


#pragma once


class PixieStreamMethod
{
public:
    typedef NeoNoSettings SettingsObject;

    PixieStreamMethod(uint16_t pixelCount, 
            size_t elementSize, 
            size_t settingsSize, 
            Stream* pixieStream) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _stream(pixieStream),
        _usEndTime(0)
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

    ~PixieStreamMethod()
    {
        _stream->flush();
        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return (micros() - _usEndTime) > 1000L; // ensure 1ms delay between calls
    }

    void Initialize()
    {
        // nothing to initialize, UART is managed outside this library
    }

    void Update(bool)
    {
        while (!IsReadyToUpdate())
        {
        }
        _stream->write(_data, _sizeData);
        _usEndTime = micros(); // Save time to ensure 1ms delay
    }

    bool AlwaysUpdate()
    {
        // Pixie expects to receive data every <2 seconds, Adafruit recommends <1.  
        // Ensuring data is sent every <2 seconds needs to happen outside of the library
        // Returning true will allow data to be re-sent even if no changes to buffer.

        return true;
    }

    bool SwapBuffers()
    {
        return false;
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    };

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const size_t   _sizeData;   // Size of '_data' buffer below

    uint8_t* _data;       // Holds LED color values
    Stream* _stream;
    uint32_t _usEndTime;  // microseconds EndTime
};
