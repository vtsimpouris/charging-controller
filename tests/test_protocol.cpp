#include <gtest/gtest.h>

extern "C"
{
#include "protocol.h"
}


TEST(ProtocolTest, CRC8MatchesKnownReferenceVector)
{
    const uint8_t data[] = {
        '1', '2', '3',
        '4', '5', '6',
        '7', '8', '9'
    };

    const uint8_t crc =
        protocol_crc8(
            data,
            sizeof(data)
        );

    EXPECT_EQ(crc, 0xF4);
}


TEST(ProtocolTest, AcceptsValidPacket)
{
    ChargingCommandPacket packet{};

    packet.start = PROTOCOL_START_BYTE;
    packet.flags =
        FLAG_VEHICLE_CONNECTED |
        FLAG_CHARGING_REQUESTED;
    packet.sequence = 1;

    const uint8_t data[] = {
        packet.start,
        packet.flags,
        packet.sequence
    };

    packet.crc =
        protocol_crc8(
            data,
            sizeof(data)
        );

    EXPECT_TRUE(
        protocol_validate_packet(&packet)
    );
}


TEST(ProtocolTest, RejectsCorruptedPacket)
{
    ChargingCommandPacket packet{};

    packet.start = PROTOCOL_START_BYTE;
    packet.flags =
        FLAG_VEHICLE_CONNECTED |
        FLAG_CHARGING_REQUESTED;
    packet.sequence = 1;

    const uint8_t data[] = {
        packet.start,
        packet.flags,
        packet.sequence
    };

    packet.crc =
        protocol_crc8(
            data,
            sizeof(data)
        );

    // Simulate corruption in transmission.
    packet.crc ^= 0x01;

    EXPECT_FALSE(
        protocol_validate_packet(&packet)
    );
}