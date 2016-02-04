/*--------------------------------------------------------------------
NeoPixel is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#pragma once

#include <Arduino.h>
#include <functional>

class NeoPixelBus;

typedef std::function<void(float progress)> AnimUpdateCallback;

#define NEO_MILLISECONDS        1    // ~65 seconds max duration, ms updates
#define NEO_CENTISECONDS       10    // ~10.9 minutes max duration, centisecond updates
#define NEO_DECISECONDS       100    // ~1.8 hours max duration, decisecond updates
#define NEO_SECONDS          1000    // ~18.2 hours max duration, second updates
#define NEO_DECASECONDS     10000    // ~7.5 days, 10 second updates

class NeoPixelAnimator
{
public:
    NeoPixelAnimator(NeoPixelBus* bus, uint16_t timeScale = NEO_MILLISECONDS);
    ~NeoPixelAnimator();

    bool IsAnimating() const
    {
        return _activeAnimations > 0;
    }

    bool IsAnimating(uint16_t n)
    {
        return (IsAnimating() && _animations[n].time != 0);
    }

    void StartAnimation(uint16_t n, uint16_t time, AnimUpdateCallback animUpdate);
    void StopAnimation(uint16_t n);
    void UpdateAnimations();

    bool IsPaused()
    {
        return (!_isRunning);
    }

    void Pause()
    {
        _isRunning = false;
    }

    void Resume()
    {
        _isRunning = true;
        _animationLastTick = millis();
    }

    uint16_t TimeScale()
    {
        return _timeScale;
    }

    void FadeTo(uint16_t time, RgbColor color);

private:
    NeoPixelBus* _bus;

    struct AnimationContext
    {
        AnimationContext() :
            time(0),
            remaining(0),
            fnUpdate(NULL)
        {}

        void StartAnimation(uint16_t duration, AnimUpdateCallback animUpdate)
        {
            time = duration;
            remaining = duration;
            fnUpdate = animUpdate;
        }

        void StopAnimation()
        {
            time = 0;
            remaining = 0;
            fnUpdate = NULL;
        }

        uint16_t time;
        uint16_t remaining;
       
        AnimUpdateCallback fnUpdate;
    };

    AnimationContext* _animations;
    uint32_t _animationLastTick;
    uint16_t _activeAnimations;
    uint16_t _timeScale;
    bool _isRunning;
};