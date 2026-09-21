#include "protocol.h"

uint8_t protocol_crc8(
    const uint8_t* data,
    uint32_t length)
{
    uint8_t crc = 0;

    for (uint32_t i = 0; i < length; ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            if (crc & 0x80)
            {
                crc = (uint8_t)((crc << 1) ^ 0x07);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

int protocol_validate_packet(
    const ChargingCommandPacket* packet)
{
    if (packet == 0)
    {
        return 0;
    }

    if (packet->start != PROTOCOL_START_BYTE)
    {
        return 0;
    }

    const uint8_t data[] = {
        packet->start,
        packet->flags,
        packet->sequence
    };

    return protocol_crc8(data, sizeof(data)) == packet->crc;
}