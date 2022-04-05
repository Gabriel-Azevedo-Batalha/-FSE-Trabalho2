#include "jsonReader.h"

void freeConfig(config *c){
	free(c->ipCentral);
	free(c->ipDist);
	free(c->name);
	free(c->inputs);
	free(c->outputs);
}

// JSON Open
char *jOpen(const char *name){

	// File Open
	FILE *file = NULL;
	file = fopen(name, "r");
	if (file == NULL) {
		printf("Falha na abertura do arquivo\n");
		exit(1);
	}

	// File Size
	struct stat statbuf;
	stat(name, &statbuf);
	int fileSize = statbuf.st_size;
	
	// File Read
	char *jsonString = (char *) malloc(sizeof(char) * fileSize + 1);
	int size = fread(jsonString, sizeof(char), fileSize, file);
	if (size == 0) {
		printf("Falha na leitura do JSON\n");
		exit(1);
	}

	fclose(file);

	return jsonString;
}

char *jParseString(cJSON *json, char *attribute){
	cJSON *item = cJSON_GetObjectItem(json, attribute);	
	return item->valuestring;
}

int jParseInt(cJSON *json, char *attribute){
	cJSON *item = cJSON_GetObjectItem(json, attribute);	
	return item->valueint;
}

deviceGpio *jParseGPIOs(cJSON *json, char *attribute, int *size, int flag){
	// WiringPi-Gpio Mapping
	int wirings[28] = {30, 31, 8, 9, 7, 21, 22, 11, 10,
					   13, 12, 14, 26, 23, 15, 16, 27, 0,
					   1, 24, 28, 29, 3, 4, 5, 6, 25, 2};
	cJSON *item = cJSON_GetObjectItem(json, attribute);
	*size = cJSON_GetArraySize(item);
	deviceGpio *devices = malloc(*size*sizeof(deviceGpio));
	for (int i = 0; i < *size; i++) {
		cJSON *obj = cJSON_GetArrayItem(item, i);	// Gets the value in the array based on the index
		devices[i].type = jParseString(obj, "type");
		devices[i].tag = jParseString(obj, "tag");
		if(flag){
			devices[i].gpio = jParseInt(obj, "gpio");
			devices[i].on = jParseInt(obj, "on");
		}
		else{
			devices[i].gpio = wirings[jParseInt(obj, "gpio")];
			devices[i].on = 0;
		}
	}
	return devices;
}

void printGPIO(deviceGpio *GPIOs, int size){
	for(int i=0;i<size;i++){
		printf("Tipo: %s, Tag: %s, GPIO: %d, ON: %d\n", GPIOs[i].type, GPIOs[i].tag, GPIOs[i].gpio, GPIOs[i].on);
	}
}

tempSensor *jParseTempSensors(cJSON *json, int *size){
	deviceGpio *devices = jParseGPIOs(json, "sensor_temperatura", size, 0);
	tempSensor *sensors = malloc(*size*sizeof(tempSensor));
	dhtData data;
	data.humidity = 0.0;
	data.temperature = 0.0;
	for(int i=0;i<*size;i++){
		sensors[i].sensor = devices[i];
		sensors[i].data = data;
	}
	return sensors;
}

void printSensors(tempSensor *GPIOs, int size){
	for(int i=0;i<size;i++){
		printf("Tipo: %s, Tag: %s, GPIO: %d, Temperatura: %f, Humidade: %f\n", GPIOs[i].sensor.type, GPIOs[i].sensor.tag, GPIOs[i].sensor.gpio, GPIOs[i].data.temperature, GPIOs[i].data.humidity);
	}
}

config readConfig(const char *file) {

	char *jsonString = jOpen(file);	

	cJSON *json = cJSON_Parse(jsonString);
	if(json == 0){
		printf("Falha na leitura do JSON\n");
		exit(1);
	}
	free(jsonString);
	
	config c;
	c.ipCentral = jParseString(json, "ip_servidor_central");
	c.portCentral = jParseInt(json, "porta_servidor_central");
	c.ipDist = jParseString(json, "ip_servidor_distribuido");
	c.portDist = jParseInt(json, "porta_servidor_distribuido");
	c.name = jParseString(json, "nome");
	c.inputs = jParseGPIOs(json, "inputs", &c.inputSize, 0);
	c.outputs = jParseGPIOs(json, "outputs", &c.outputSize, 0);
	c.tSensors = jParseTempSensors(json, &c.sensorSize);
	c.counter = 0;

	free(json);

	return c;
}

deviceGpio *readInstructions(const char *buffer){

	cJSON *json = cJSON_Parse(buffer);
	if(json == 0){
		printf("Falha na leitura do JSON\n");
		printf("BUFFER JSON: %s\n", buffer);
		exit(1);
	}
	int size;
	deviceGpio *devices = jParseGPIOs(json, "outputs", &size, 1);
	return devices;
}

void gpioToJson(deviceGpio *devices, int size, cJSON *array){
	for(int i=0;i<size;i++){
		cJSON *gpio = cJSON_CreateObject();
		cJSON_AddItemToArray(array, gpio);
		cJSON_AddItemToObject(gpio, "type", cJSON_CreateString(devices[i].type));
		cJSON_AddItemToObject(gpio, "tag", cJSON_CreateString(devices[i].tag));
		cJSON_AddItemToObject(gpio, "gpio",cJSON_CreateNumber(devices[i].gpio));
		cJSON_AddItemToObject(gpio, "on",cJSON_CreateNumber(devices[i].on));
	}
		//cJSON_Delete(gpio);
}

void sensorToJson(deviceGpio *devices, dhtData *datas, int size, cJSON *array){
	for(int i=0;i<size;i++){
		cJSON *sensor = cJSON_CreateObject();
		cJSON_AddItemToArray(array, sensor);

		cJSON *gpio = cJSON_CreateObject();
		cJSON_AddItemToObject(sensor, "sensor", gpio);

		cJSON_AddItemToObject(gpio, "type", cJSON_CreateString(devices[i].type));
		cJSON_AddItemToObject(gpio, "tag", cJSON_CreateString(devices[i].tag));
		cJSON_AddItemToObject(gpio, "gpio",cJSON_CreateNumber(devices[i].gpio));
		cJSON_AddItemToObject(gpio, "on",cJSON_CreateNumber(devices[i].on));
		cJSON *data = cJSON_CreateObject();
		cJSON_AddItemToObject(sensor, "data", data);
		cJSON_AddItemToObject(data, "temperature", cJSON_CreateNumber(datas[i].temperature));
		cJSON_AddItemToObject(data, "humidity", cJSON_CreateNumber(datas[i].humidity));
	}
}

char *configToString(config c){
	cJSON *json = cJSON_CreateObject();
	cJSON_AddItemToObject(json, "ip_servidor_central", cJSON_CreateString(c.ipCentral));
	cJSON_AddItemToObject(json, "porta_servidor_central", cJSON_CreateNumber(c.portCentral));
	cJSON_AddItemToObject(json, "ip_servidor_distribuido", cJSON_CreateString(c.ipDist));
	cJSON_AddItemToObject(json, "porta_servidor_distribuido", cJSON_CreateNumber(c.portDist));
	cJSON_AddItemToObject(json, "nome", cJSON_CreateString(c.name));
	cJSON_AddItemToObject(json, "counter", cJSON_CreateNumber(c.counter));

	cJSON *outputs = cJSON_CreateArray();	
	cJSON_AddItemToObject(json, "outputs", outputs);
	gpioToJson(c.outputs, c.outputSize, outputs);
	
	cJSON *inputs = cJSON_CreateArray();	
	cJSON_AddItemToObject(json, "inputs", inputs);
	gpioToJson(c.inputs, c.inputSize, inputs);

	cJSON *sensors = cJSON_CreateArray();
	cJSON_AddItemToObject(json, "sensor_temperatura", sensors);
	deviceGpio *tSensors = malloc(c.sensorSize*sizeof(deviceGpio));
	dhtData *tData = malloc(c.sensorSize*sizeof(dhtData));
	for(int i=0;i<c.sensorSize;i++){
		tSensors[i] = c.tSensors[i].sensor;
		tData[i] = c.tSensors[i].data;
	}
	sensorToJson(tSensors, tData, c.sensorSize, sensors);
	free(tSensors);
	free(tData);
	char *str = cJSON_PrintUnformatted(json);

	//char *str = cJSON_Print(json);
	//printf("%s\n", str);
	return str;
}

