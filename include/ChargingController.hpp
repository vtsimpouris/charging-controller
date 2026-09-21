#pragma once

#include <cstdint>

#include "ChargerState.hpp"
#include "Fault.hpp"
#include "Telemetry.hpp"

class ChargingController
{
public:
    ChargingController() = default;

    void update(const Telemetry& telemetry, std::uint32_t deltaMs);
    void reset();

    ChargerState getState() const;
    Fault getFault() const;
    bool shouldCloseContactor() const;

private:
    ChargerState state_{ChargerState::Idle};
    Fault fault_{Fault::None};
    bool contactorClosed_{false};

    std::uint32_t communicationLostMs_{0};

    static constexpr float MAX_VOLTAGE = 450.0f;
    static constexpr float MAX_CURRENT = 200.0f;
    static constexpr float MAX_TEMPERATURE = 80.0f;

    static constexpr std::uint32_t COMMUNICATION_TIMEOUT_MS = 100;

    void checkFaults(
        const Telemetry& telemetry,
        std::uint32_t deltaMs
    );
};