#pragma once

#include <stdint.h>

static const uint8_t kGatewayMacAddress[6] = {0xF8, 0x5B, 0x1B, 0x05, 0x71, 0xB4}; // Adresse de la passerelle (écriture hexadécimale)
constexpr uint8_t kWifiChannel = 7; // Canal wifi de la passerelle

#define DEVICE_NAME "sensor_n" // modifiez cette valeur avec le nom de votre capteur (sensor_<groupe>), ex : "sensor_a"