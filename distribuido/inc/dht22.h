#ifndef DHT22_H
#define DHT22_H

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


typedef struct dhtData{
	double temperature;
	double humidity;
} dhtData;

dhtData read_dht_data();

#endif
