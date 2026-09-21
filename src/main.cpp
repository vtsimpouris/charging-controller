#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <nlohmann/json.hpp>

#include "ChargingController.hpp"
#include "CurrentController.hpp"
#include "hal.h"
#include "sim_hal.h"


struct ControlParameters
{
    float kp;
    std::uint32_t controlPeriodMs;
};


ControlParameters loadControlParameters()
{
    std::ifstream file("config/control_parameters.json");

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open config/control_parameters.json"
        );
    }

    nlohmann::json json;
    file >> json;

    return {
        .kp = json.at("kp").get<float>(),
        .controlPeriodMs =
            json.at("control_period_ms").get<std::uint32_t>()
    };
}


const char* stateToString(ChargerState state)
{
    switch (state)
    {
        case ChargerState::Idle:
            return "IDLE";

        case ChargerState::Connected:
            return "CONNECTED";

        case ChargerState::Charging:
            return "CHARGING";

        case ChargerState::Complete:
            return "COMPLETE";

        case ChargerState::Fault:
            return "FAULT";
    }

    return "UNKNOWN";
}


const char* faultToString(Fault fault)
{
    switch (fault)
    {
        case Fault::None:
            return "NONE";

        case Fault::OverVoltage:
            return "OVER_VOLTAGE";

        case Fault::OverCurrent:
            return "OVER_CURRENT";

        case Fault::OverTemperature:
            return "OVER_TEMPERATURE";

        case Fault::CommunicationTimeout:
            return "COMMUNICATION_TIMEOUT";
    }

    return "UNKNOWN";
}


void runCycle(
    ChargingController& controller,
    std::uint32_t controlPeriodMs)
{
    /*
     * Advance simulated hardware time before
     * executing one control-loop iteration.
     */
    sim_hal_advance_time(controlPeriodMs);

    Telemetry telemetry{
        .voltage = hal_read_voltage(),
        .current = hal_read_current(),
        .temperature = hal_read_temperature(),
        .vehicleConnected = hal_vehicle_connected() != 0,
        .chargingRequested = hal_charging_requested() != 0,
        .communicationAlive = hal_communication_alive() != 0
    };

    controller.update(
        telemetry,
        controlPeriodMs
    );

    hal_set_contactor(
        controller.shouldCloseContactor() ? 1 : 0
    );

    /*
     * A healthy control loop services the watchdog
     * once per cycle.
     */
    hal_kick_watchdog();

    std::cout
        << "Voltage: " << telemetry.voltage << " V"
        << " | Current: " << telemetry.current << " A"
        << " | Temperature: " << telemetry.temperature << " C"
        << " | State: " << stateToString(controller.getState())
        << " | Fault: " << faultToString(controller.getFault())
        << '\n';

    std::this_thread::sleep_for(
        std::chrono::milliseconds(controlPeriodMs)
    );
}


void demonstrateCurrentControl(
    const ControlParameters& parameters)
{
    std::cout
        << "\n--- Closed-loop current control ---\n";

    sim_hal_reset();
    sim_hal_set_current(0.0f);

    constexpr float TARGET_CURRENT = 120.0f;

    CurrentController currentController(
        parameters.kp
    );

    std::cout
        << "Kp: " << parameters.kp
        << " | Control period: "
        << parameters.controlPeriodMs
        << " ms\n";

    for (int cycle = 0; cycle < 150; ++cycle)
    {
        const float measuredCurrent =
            hal_read_current();

        const float controlInput =
            currentController.update(
                TARGET_CURRENT,
                measuredCurrent
            );

        sim_hal_step_current_plant(
            controlInput,
            parameters.controlPeriodMs
        );

        if (cycle % 10 == 0)
        {
            std::cout
                << "t = "
                << cycle * parameters.controlPeriodMs
                << " ms"
                << " | Target: "
                << TARGET_CURRENT
                << " A"
                << " | Current: "
                << hal_read_current()
                << " A"
                << " | Control: "
                << controlInput
                << '\n';
        }
    }
}


int main()
{
    const ControlParameters parameters =
        loadControlParameters();

    ChargingController controller;

    sim_hal_reset();


    /*
     * Healthy startup
     */
    std::cout << "\n--- Healthy startup ---\n";

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );


    /*
     * Immediate overcurrent safety fault
     */
    std::cout
        << "\n--- Injecting overcurrent ---\n";

    sim_hal_set_current(250.0f);

    runCycle(
        controller,
        parameters.controlPeriodMs
    );


    /*
     * Immediate overtemperature safety fault
     */
    std::cout
        << "\n--- Injecting overtemperature ---\n";

    controller.reset();
    sim_hal_reset();

    sim_hal_set_temperature(95.0f);

    runCycle(
        controller,
        parameters.controlPeriodMs
    );


    /*
     * CRC-corrupted vehicle communication.
     *
     * First reach a normal charging state.
     * Then corrupt the incoming packet and allow
     * the communication timeout to accumulate.
     */
    std::cout
        << "\n--- Injecting corrupted vehicle communication ---\n";

    controller.reset();
    sim_hal_reset();

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    sim_hal_corrupt_vehicle_packet();

    for (int i = 0; i < 6; ++i)
    {
        runCycle(
            controller,
            parameters.controlPeriodMs
        );
    }


    /*
     * Simulated control-task stall.
     *
     * The software no longer executes, so the watchdog
     * is not serviced. The HAL therefore forces the
     * contactor open independently of the controller.
     */
    std::cout
        << "\n--- Simulating stalled control loop ---\n";

    controller.reset();
    sim_hal_reset();

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    runCycle(
        controller,
        parameters.controlPeriodMs
    );

    std::cout
        << "Control task stalled...\n";

    /*
     * Simulate 120 ms passing without executing
     * the control task or servicing the watchdog.
     */
    sim_hal_advance_time(120);

    if (sim_hal_watchdog_expired())
    {
        std::cout
            << "Watchdog reset required - "
               "system forced safe\n";
    }


    /*
     * Closed-loop current-control demonstration
     * using the gain and control period loaded
     * from control_parameters.json.
     */
    demonstrateCurrentControl(parameters);


    return 0;
}