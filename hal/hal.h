#ifndef HAL_H
#define HAL_H

#ifdef __cplusplus
extern "C" {
#endif

float hal_read_voltage(void);
float hal_read_current(void);
float hal_read_temperature(void);

int hal_vehicle_connected(void);
int hal_charging_requested(void);
int hal_communication_alive(void);

void hal_set_contactor(int closed);
void hal_kick_watchdog(void);

#ifdef __cplusplus
}
#endif

#endif