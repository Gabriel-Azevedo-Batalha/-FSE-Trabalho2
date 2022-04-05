#ifndef JSON_READER_H
#define JSON_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cJSON.h"

typedef struct dhtData{
	double temperature;
	double humidity;
} dhtData;

typedef struct deviceGpio{
	char *type;
	char *tag;
	int gpio;
	int on;
} deviceGpio;

typedef struct tempSensor{
	deviceGpio sensor;
	dhtData data;
} tempSensor;


typedef struct config{
	char *ipCentral;
	int portCentral;
	char *ipDist;
	int portDist;
	char *name;
	deviceGpio *inputs;
	int inputSize;
	deviceGpio *outputs;
	int outputSize;
	tempSensor *tSensors;
	int sensorSize;
	int counter;
} config;

void freeConfig(config *c);
char *jOpen(const char *name);
char *jParseString(cJSON *json, char *attribute);
int jParseInt(cJSON *json, char *attribute);
tempSensor *jParseTempSensor(cJSON *json, char *attribute, int *size);
deviceGpio *jParseGPIOs(cJSON *json, char *attribute, int *size);
config readConfig(char *jsonString);
void gpioToJson(deviceGpio *devices, int size, cJSON *array);
char *configToString(config c);

#endif
