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

#include "NeoPixelBus.h"
#include "NeoPixelAnimator.h"

NeoPixelAnimator::NeoPixelAnimator(NeoPixelBus* bus) :
    _bus(bus),
    _animationLastTick(0),
    _activeAnimations(0),
    _isRunning(true)
{
    _animations = new AnimationContext[_bus->PixelCount()];
}

NeoPixelAnimator::~NeoPixelAnimator()
{
    _bus = NULL;
    if (_animations)
    {
        delete[] _animations;
        _animations = NULL;
    }
}

void NeoPixelAnimator::StartAnimation(uint16_t n, uint16_t time, AnimUpdateCallback animUpdate)
{
    if (n >= _bus->PixelCount())
    {
        return;
    }

    if (_activeAnimations == 0)
    {
        _animationLastTick = millis();
    }

    StopAnimation(n);

    if (time == 0)
    {
        time = 1;
    }

    _animations[n].time = time;
    _animations[n].remaining = time;
    _animations[n].fnUpdate = animUpdate;

    _activeAnimations++;
}

void NeoPixelAnimator::StopAnimation(uint16_t n)
{
    if (IsAnimating(n))
    {
        _activeAnimations--;
        _animations[n].time = 0;
        _animations[n].remaining = 0;
        _animations[n].fnUpdate = NULL;
    }
}

void NeoPixelAnimator::FadeTo(uint16_t time, RgbColor color)
{
    for (uint16_t n = 0; n < _bus->PixelCount(); n++)
    {
        RgbColor original = _bus->GetPixelColor(n);
        AnimUpdateCallback animUpdate = [=](float progress)
        {
            RgbColor updatedColor = RgbColor::LinearBlend(original, color, progress);
            _bus->SetPixelColor(n, updatedColor);
        };
        StartAnimation(n, time, animUpdate);
    }
}

void NeoPixelAnimator::UpdateAnimations(uint32_t maxDeltaMs)
{
    if (_isRunning)
    {
        uint32_t currentTick = millis();
        uint32_t delta = currentTick - _animationLastTick;

        if (delta > maxDeltaMs)
        {
            delta = maxDeltaMs;
        }

        if (delta > 0)
        {
            uint16_t countAnimations = _activeAnimations;

            AnimationContext* pAnim;

            for (uint16_t iAnim = 0; iAnim < _bus->PixelCount() && countAnimations > 0; iAnim++)
            {
                pAnim = &_animations[iAnim];

                if (pAnim->remaining > delta)
                {
                    pAnim->remaining -= delta;

                    float progress = (float)(pAnim->time - pAnim->remaining) / (float)pAnim->time;

                    pAnim->fnUpdate(progress);
                    countAnimations--;
                }
                else if (pAnim->remaining > 0)
                {
                    pAnim->fnUpdate(1.0f);
                    StopAnimation(iAnim);
                    countAnimations--;
                }
            }

            _animationLastTick = currentTick;
        }
    }
}

