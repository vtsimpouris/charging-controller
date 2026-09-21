#include "ChargingController.hpp"

void ChargingController::update(
    const Telemetry& telemetry,
    std::uint32_t deltaMs)
{
    checkFaults(telemetry, deltaMs);

    if (fault_ != Fault::None)
    {
        state_ = ChargerState::Fault;
        contactorClosed_ = false;
        return;
    }

    switch (state_)
    {
        case ChargerState::Idle:
            contactorClosed_ = false;

            if (telemetry.vehicleConnected)
                state_ = ChargerState::Connected;

            break;

        case ChargerState::Connected:
            contactorClosed_ = false;

            if (!telemetry.vehicleConnected)
                state_ = ChargerState::Idle;
            else if (telemetry.chargingRequested)
                state_ = ChargerState::Charging;

            break;

        case ChargerState::Charging:
            contactorClosed_ = true;

            if (!telemetry.vehicleConnected)
            {
                state_ = ChargerState::Idle;
                contactorClosed_ = false;
            }
            else if (!telemetry.chargingRequested)
            {
                state_ = ChargerState::Complete;
                contactorClosed_ = false;
            }

            break;

        case ChargerState::Complete:
            contactorClosed_ = false;

            if (!telemetry.vehicleConnected)
                state_ = ChargerState::Idle;

            break;

        case ChargerState::Fault:
            contactorClosed_ = false;
            break;
    }
}

void ChargingController::checkFaults(
    const Telemetry& telemetry,
    std::uint32_t deltaMs)
{
    fault_ = Fault::None;

    if (telemetry.communicationAlive)
    {
        communicationLostMs_ = 0;
    }
    else
    {
        communicationLostMs_ += deltaMs;
    }

    if (telemetry.current > MAX_CURRENT)
    {
        fault_ = Fault::OverCurrent;
    }
    else if (telemetry.voltage > MAX_VOLTAGE)
    {
        fault_ = Fault::OverVoltage;
    }
    else if (telemetry.temperature > MAX_TEMPERATURE)
    {
        fault_ = Fault::OverTemperature;
    }
    else if (communicationLostMs_ >= COMMUNICATION_TIMEOUT_MS)
    {
        fault_ = Fault::CommunicationTimeout;
    }
}

void ChargingController::reset()
{
    state_ = ChargerState::Idle;
    fault_ = Fault::None;
    contactorClosed_ = false;
    communicationLostMs_ = 0;
}

ChargerState ChargingController::getState() const
{
    return state_;
}

Fault ChargingController::getFault() const
{
    return fault_;
}

bool ChargingController::shouldCloseContactor() const
{
    return contactorClosed_;
}