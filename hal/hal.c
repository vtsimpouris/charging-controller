#include "hal.h"
#include "sim_hal.h"
#include "protocol.h"

#include <stdint.h>
#include <stdio.h>


#define WATCHDOG_TIMEOUT_MS 100


/* Simulated local sensor values */
static float voltage = 400.0f;
static float current = 120.0f;
static float temperature = 45.0f;
static float current_rate = 0.0f;

/* Last valid vehicle state received through communication */
static int last_valid_vehicle_connected = 1;
static int last_valid_charging_requested = 1;


/* Simulated actuator state */
static int contactor_closed = 0;


/* Simulated watchdog state */
static uint32_t watchdog_elapsed_ms = 0;
static int watchdog_expired = 0;


/* Simulated incoming vehicle communication packet */
static ChargingCommandPacket vehicle_packet;


/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static void update_vehicle_packet_crc(void)
{
    const uint8_t data[] = {
        vehicle_packet.start,
        vehicle_packet.flags,
        vehicle_packet.sequence
    };

    vehicle_packet.crc =
        protocol_crc8(data, sizeof(data));
}


static void process_vehicle_packet(void)
{
    if (!protocol_validate_packet(&vehicle_packet))
    {
        /*
         * Invalid packet:
         * keep using the last valid vehicle state.
         *
         * Communication health itself is reported separately by
         * hal_communication_alive().
         */
        return;
    }

    last_valid_vehicle_connected =
        (vehicle_packet.flags & FLAG_VEHICLE_CONNECTED) != 0;

    last_valid_charging_requested =
        (vehicle_packet.flags & FLAG_CHARGING_REQUESTED) != 0;
}


/* -------------------------------------------------------------------------- */
/* Hardware Abstraction Layer                                                 */
/* -------------------------------------------------------------------------- */

float hal_read_voltage(void)
{
    return voltage;
}


float hal_read_current(void)
{
    return current;
}


float hal_read_temperature(void)
{
    return temperature;
}


int hal_vehicle_connected(void)
{
    process_vehicle_packet();

    return last_valid_vehicle_connected;
}


int hal_charging_requested(void)
{
    process_vehicle_packet();

    return last_valid_charging_requested;
}


int hal_communication_alive(void)
{
    return protocol_validate_packet(&vehicle_packet);
}


void hal_set_contactor(int closed)
{
    contactor_closed = closed;

    printf(
        "HAL: Contactor %s\n",
        contactor_closed ? "CLOSED" : "OPEN"
    );
}


void hal_kick_watchdog(void)
{
    watchdog_elapsed_ms = 0;

    printf("HAL: Watchdog serviced\n");
}


/* -------------------------------------------------------------------------- */
/* Simulation interface                                                       */
/* -------------------------------------------------------------------------- */

void sim_hal_reset(void)
{
    /* Healthy local sensor state */
    voltage = 400.0f;
    current = 120.0f;
    temperature = 45.0f;
    current_rate = 0.0f;

    /* Healthy vehicle communication packet */
    vehicle_packet.start = PROTOCOL_START_BYTE;

    vehicle_packet.flags =
        FLAG_VEHICLE_CONNECTED |
        FLAG_CHARGING_REQUESTED;

    vehicle_packet.sequence = 0;

    update_vehicle_packet_crc();

    /*
     * Last known valid communication state.
     * Used if a later packet becomes corrupted.
     */
    last_valid_vehicle_connected = 1;
    last_valid_charging_requested = 1;

    /* Reset watchdog */
    watchdog_elapsed_ms = 0;
    watchdog_expired = 0;

    /* Hardware starts in safe state */
    contactor_closed = 0;
}

void sim_hal_step_current_plant(
    float control_input,
    uint32_t delta_ms)
{
    const float dt = (float)delta_ms / 1000.0f;

    /*
     * Simplified plant:
     *
     *     G(s) = 1 / (s(s + 2))
     *
     * Equivalent state equations:
     *
     *     dI/dt = current_rate
     *     d(current_rate)/dt = control_input - 2 * current_rate
     */

    const float acceleration =
        control_input - 2.0f * current_rate;

    current_rate += acceleration * dt;
    current += current_rate * dt;
}


void sim_hal_set_voltage(float value)
{
    voltage = value;
}


void sim_hal_set_current(float value)
{
    current = value;
}


void sim_hal_set_temperature(float value)
{
    temperature = value;
}


void sim_hal_set_vehicle_packet(
    int connected,
    int charging_requested)
{
    vehicle_packet.start = PROTOCOL_START_BYTE;
    vehicle_packet.sequence++;

    vehicle_packet.flags = 0;

    if (connected)
    {
        vehicle_packet.flags |= FLAG_VEHICLE_CONNECTED;
    }

    if (charging_requested)
    {
        vehicle_packet.flags |= FLAG_CHARGING_REQUESTED;
    }

    update_vehicle_packet_crc();
}


void sim_hal_corrupt_vehicle_packet(void)
{
    /*
     * Deliberately corrupt the CRC without modifying
     * or recalculating the packet payload.
     */
    vehicle_packet.crc ^= 0x01;
}


void sim_hal_advance_time(uint32_t delta_ms)
{
    if (watchdog_expired)
    {
        return;
    }

    watchdog_elapsed_ms += delta_ms;

    if (watchdog_elapsed_ms >= WATCHDOG_TIMEOUT_MS)
    {
        watchdog_expired = 1;

        /*
         * Simulated hardware watchdog fails safe:
         * power contactor is forced open.
         */
        contactor_closed = 0;

        printf(
            "HAL: WATCHDOG EXPIRED - forcing contactor OPEN\n"
        );
    }
}


int sim_hal_watchdog_expired(void)
{
    return watchdog_expired;
}