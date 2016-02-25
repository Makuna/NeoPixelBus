/*-------------------------------------------------------------------------
NeoPixelAnimator provides animation timing support.

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

#include "NeoPixelAnimator.h"

NeoPixelAnimator::NeoPixelAnimator(uint16_t countAnimations, uint16_t timeScale) :
    _countAnimations(countAnimations),
    _animationLastTick(0),
    _activeAnimations(0),
    _isRunning(true)
{
    // due to strange esp include header issues, min and max aren't usable
    _timeScale = (timeScale < 1) ? (1) : (timeScale > 32768) ? 32768 : timeScale;
    //_timeScale = max(1, min(32768, timeScale));

    _animations = new AnimationContext[_countAnimations];
}

NeoPixelAnimator::~NeoPixelAnimator()
{
    delete[] _animations;
}

bool NeoPixelAnimator::NextAvailableAnimation(uint16_t* result, uint16_t after)
{
    if (result == NULL)
    {
        return false;
    }

    if (after >= _countAnimations)
    {
        // last one
        after = _countAnimations - 1;
    }

    uint16_t next = after;

    do
    {
        next = (next + 1) % _countAnimations;
        if (!IsAnimationActive(next))
        {
            *result = next;
            return true;
        }
    } while (next != after);
    return false;
}

void NeoPixelAnimator::StartAnimation(uint16_t n, 
        uint16_t duration, 
        AnimUpdateCallback animUpdate)
{
    if (n >= _countAnimations || animUpdate == NULL)
    {
        return;
    }

    if (_activeAnimations == 0)
    {
        _animationLastTick = millis();
    }

    StopAnimation(n);

    // all animations must have at least non zero duration, otherwise
    // they are considered stopped
    if (duration == 0)
    {
        duration = 1;
    }

    _animations[n].StartAnimation(duration, animUpdate);

    _activeAnimations++;
}

void NeoPixelAnimator::StopAnimation(uint16_t n)
{
    if (n >= _countAnimations)
    {
        return;
    }

    if (IsAnimationActive(n))
    {
        _activeAnimations--;
        _animations[n].StopAnimation();
    }
}

void NeoPixelAnimator::UpdateAnimations()
{
    if (_isRunning)
    {
        uint32_t currentTick = millis();
        uint32_t delta = currentTick - _animationLastTick;

        if (delta >= _timeScale)
        {
            uint16_t countAnimations = _activeAnimations;
            AnimationContext* pAnim;

            delta /= _timeScale; // scale delta into animation time

            for (uint16_t iAnim = 0; iAnim < _countAnimations && countAnimations > 0; iAnim++)
            {
                pAnim = &_animations[iAnim];
                AnimUpdateCallback fnUpdate = pAnim->_fnCallback;
                AnimationParam param;
                
                param.index = iAnim;

                if (pAnim->_remaining > delta)
                {
                    param.state = (pAnim->_remaining == pAnim->_duration) ? AnimationState_Started : AnimationState_Progress;
                    param.progress = (float)(pAnim->_duration - pAnim->_remaining) / (float)pAnim->_duration;

                    fnUpdate(param);

                    pAnim->_remaining -= delta;

                    countAnimations--;
                }
                else if (pAnim->_remaining > 0)
                {
                    param.state = AnimationState_Completed;
                    param.progress = 1.0f;

                    _activeAnimations--; 
                    pAnim->StopAnimation();

                    fnUpdate(param);
                    
                    countAnimations--;
                }
            }

            _animationLastTick = currentTick;
        }
    }
}
