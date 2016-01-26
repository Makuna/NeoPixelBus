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

class NeoPixelAnimator
{
public:
    NeoPixelAnimator(NeoPixelBus* bus);
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
    void UpdateAnimations(uint32_t maxDeltaMs = 1000);

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

        uint16_t time;
        uint16_t remaining;
       
        AnimUpdateCallback fnUpdate;
    };

    AnimationContext* _animations;
    uint32_t _animationLastTick;
    uint16_t _activeAnimations;
    bool _isRunning;
};