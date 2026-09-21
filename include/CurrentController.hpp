#pragma once

class CurrentController
{
public:
    explicit CurrentController(float gain);

    float update(float referenceCurrent,
                 float measuredCurrent) const;

private:
    float gain_;
};