#ifndef SENSORDATA_H
#define SENSORDATA_H

#define EXPECTED_DATA_HEADER "DataPack"
#define CHECKSUM_SIZE 12
#define HEADER_SIZE 8

// The order of sensors is:
/*
 * thermocouple[0] = injector plate & kerosene inlet
 * thermocouple[1] = injector plate & oxidizer inlet
 * thermocouple[2] = outside the cc at the throat
 * thermocouple[3] = on the nozzle near the outlet
 *
 * pressureTransducer[0] = combustion chamber
 * pressureTransducer[1] = kerosene feed-line pressure
 * pressureTransducer[2] = kerosene tank pressure
 * pressureTransducer[3] = kerosene line pressure
 * pressureTransducer[4] = oxidizer tank pressure
 * pressureTransducer[5] = oxidizer line pressure
*/

typedef struct {
    char header[HEADER_SIZE];
    float loadCell;
    float thermocouple[4];
    float pressureTransducer[6];
    char checksum[CHECKSUM_SIZE];
} SensorData;

#endif // SENSORDATA_H
