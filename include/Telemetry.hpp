#pragma once

struct Telemetry
{
    float voltage;
    float current;
    float temperature;

    bool vehicleConnected;
    bool chargingRequested;
    bool communicationAlive;
};