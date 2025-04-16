#include "AccelStepper.h"
#include "MultiStepper.h"

class ReactiveMultiStepper : public MultiStepper
{
public:
    // immeditately update targets/override any ongoing motion
    void setNewTargets(long absolute[])
    {
        for (uint8_t i = 0; i < _num_steppers; i++)
        {
            _steppers[i]->stop();
            _steppers[i]->setCurrentPosition(_steppers[i]->currentPosition());
        }
        // calc new motion, from og MultiStepper
        moveTo(absolute);
    }

    // non-blocking version of runSpeedToPosition()
    bool runAsync()
    {
        return run(); // TODO: using original run() but don't block, does this work?
    }
};