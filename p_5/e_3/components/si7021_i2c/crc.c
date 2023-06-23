/* Si7021 CRC Checksum */

#include <stdint.h> // uint8_t

#define CRC_POLY 0x31
#define CRC_INIT 0x00
#define CRC_XOR  0x00

static uint8_t _crc8(
    uint8_t *data,
    uint8_t len
) {
    uint8_t crc = CRC_INIT;
    uint8_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc ^ CRC_XOR;
}