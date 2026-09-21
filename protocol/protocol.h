#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t start;
    uint8_t flags;
    uint8_t sequence;
    uint8_t crc;
} ChargingCommandPacket;

enum
{
    PROTOCOL_START_BYTE = 0xA5,

    FLAG_VEHICLE_CONNECTED = 1 << 0,
    FLAG_CHARGING_REQUESTED = 1 << 1
};

uint8_t protocol_crc8(
    const uint8_t* data,
    uint32_t length
);

int protocol_validate_packet(
    const ChargingCommandPacket* packet
);

#ifdef __cplusplus
}
#endif

#endif