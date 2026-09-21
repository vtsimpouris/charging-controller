#include "CurrentController.hpp"

CurrentController::CurrentController(float gain)
    : gain_(gain)
{
}

float CurrentController::update(
    float referenceCurrent,
    float measuredCurrent) const
{
    const float error =
        referenceCurrent - measuredCurrent;

    return gain_ * error;
}