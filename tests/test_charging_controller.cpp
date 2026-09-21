#include <gtest/gtest.h>

#include "ChargingController.hpp"
#include "Telemetry.hpp"


namespace
{
Telemetry healthyTelemetry()
{
    return {
        .voltage = 400.0f,
        .current = 120.0f,
        .temperature = 45.0f,
        .vehicleConnected = true,
        .chargingRequested = true,
        .communicationAlive = true
    };
}

constexpr std::uint32_t PERIOD_MS = 20;
}


TEST(ChargingControllerTest, StartsInSafeIdleState)
{
    ChargingController controller;

    EXPECT_EQ(controller.getState(), ChargerState::Idle);
    EXPECT_EQ(controller.getFault(), Fault::None);
    EXPECT_FALSE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, ProgressesToCharging)
{
    ChargingController controller;
    const Telemetry telemetry = healthyTelemetry();

    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Connected
    );

    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Charging
    );

    // Contactor closes on the next Charging cycle.
    controller.update(telemetry, PERIOD_MS);

    EXPECT_TRUE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, DetectsOvercurrent)
{
    ChargingController controller;

    Telemetry telemetry = healthyTelemetry();
    telemetry.current = 250.0f;

    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getFault(),
        Fault::OverCurrent
    );

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Fault
    );

    EXPECT_FALSE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, DetectsOvervoltage)
{
    ChargingController controller;

    Telemetry telemetry = healthyTelemetry();
    telemetry.voltage = 500.0f;

    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getFault(),
        Fault::OverVoltage
    );

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Fault
    );

    EXPECT_FALSE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, DetectsOvertemperature)
{
    ChargingController controller;

    Telemetry telemetry = healthyTelemetry();
    telemetry.temperature = 95.0f;

    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getFault(),
        Fault::OverTemperature
    );

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Fault
    );

    EXPECT_FALSE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, CommunicationTimeoutRequires100Milliseconds)
{
    ChargingController controller;

    Telemetry telemetry = healthyTelemetry();

    // Reach normal charging state first.
    controller.update(telemetry, PERIOD_MS);
    controller.update(telemetry, PERIOD_MS);
    controller.update(telemetry, PERIOD_MS);

    telemetry.communicationAlive = false;

    // 80 ms of communication loss must not fault yet.
    for (int i = 0; i < 4; ++i)
    {
        controller.update(telemetry, PERIOD_MS);
    }

    EXPECT_EQ(
        controller.getFault(),
        Fault::None
    );

    // Fifth 20 ms interval reaches 100 ms.
    controller.update(telemetry, PERIOD_MS);

    EXPECT_EQ(
        controller.getFault(),
        Fault::CommunicationTimeout
    );

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Fault
    );

    EXPECT_FALSE(controller.shouldCloseContactor());
}


TEST(ChargingControllerTest, ResetClearsFaultAndReturnsToIdle)
{
    ChargingController controller;

    Telemetry telemetry = healthyTelemetry();
    telemetry.current = 250.0f;

    controller.update(telemetry, PERIOD_MS);

    ASSERT_EQ(
        controller.getState(),
        ChargerState::Fault
    );

    controller.reset();

    EXPECT_EQ(
        controller.getState(),
        ChargerState::Idle
    );

    EXPECT_EQ(
        controller.getFault(),
        Fault::None
    );

    EXPECT_FALSE(controller.shouldCloseContactor());
}