#ifndef DHT22_H
#define DHT22_H

#define MAX_TIMINGS	85

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    float celsiusTemperature;
    float humidity;
} DHT22;

int read_dht_data(DHT22 *dht22, uint8_t dht_pin);

#endif
