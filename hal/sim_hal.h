#ifndef SIM_HAL_H
#define SIM_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void sim_hal_reset(void);

void sim_hal_set_voltage(float value);
void sim_hal_set_current(float value);
void sim_hal_set_temperature(float value);

void sim_hal_set_vehicle_packet(
    int connected,
    int charging_requested
);

void sim_hal_corrupt_vehicle_packet(void);

void sim_hal_step_current_plant(
    float control_input,
    uint32_t delta_ms
);

void sim_hal_advance_time(uint32_t delta_ms);
int sim_hal_watchdog_expired(void);

#ifdef __cplusplus
}
#endif

#endif