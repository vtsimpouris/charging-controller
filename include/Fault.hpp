#pragma once

enum class Fault
{
    None,
    OverVoltage,
    OverCurrent,
    OverTemperature,
    CommunicationTimeout
};