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
	printf("FileSize : %d\n", fileSize);


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

double jParseDouble(cJSON *json, char *attribute){
	cJSON *item = cJSON_GetObjectItem(json, attribute);	
	return item->valuedouble;
}

deviceGpio *jParseGPIOs(cJSON *json, char *attribute, int *size){
	cJSON *item = cJSON_GetObjectItem(json, attribute);
	*size = cJSON_GetArraySize(item);
	deviceGpio *devices = malloc(*size*sizeof(deviceGpio));
	for (int i = 0; i < *size; i++) {
		cJSON *obj = cJSON_GetArrayItem(item, i);	// Gets the value in the array based on the index
		devices[i].type = jParseString(obj, "type");
		devices[i].tag = jParseString(obj, "tag");
		devices[i].gpio = jParseInt(obj, "gpio");
		devices[i].on = jParseInt(obj, "on");
	}
	return devices;
}

tempSensor *jParseTempSensors(cJSON *json, int *size){
	cJSON *item = cJSON_GetObjectItem(json, "sensor_temperatura");
	*size = cJSON_GetArraySize(item);
	deviceGpio *devices = malloc(*size*sizeof(deviceGpio));
	dhtData *datas = malloc(*size*sizeof(dhtData));
	for (int i = 0; i < *size; i++) {
		cJSON *obj = cJSON_GetArrayItem(item, i);	// Gets the value in the array based on the index
		cJSON *piece = cJSON_GetObjectItem(obj, "sensor");
		devices[i].type = jParseString(piece, "type");
		devices[i].tag = jParseString(piece, "tag");
		devices[i].gpio = jParseInt(piece, "gpio");
		devices[i].on = jParseInt(piece, "on");
		piece = cJSON_GetObjectItem(obj, "data");
		datas[i].temperature = jParseDouble(piece, "temperature");
		datas[i].humidity = jParseDouble(piece, "humidity");
	}

	tempSensor *sensors = malloc(*size*sizeof(tempSensor));
	for(int i=0;i<*size;i++){
		sensors[i].sensor = devices[i];
		sensors[i].data = datas[i];
	}
	return sensors;
}

config readConfig(char *jsonString) {

	cJSON *json = cJSON_Parse(jsonString);
	if(json == 0){
		//printf("Falha na leitura do JSON\n");
		config x;
		x.name = NULL;
		return x;
	}
	free(jsonString);

	config c;
	c.ipCentral = jParseString(json, "ip_servidor_central");
	c.portCentral = jParseInt(json, "porta_servidor_central");
	c.ipDist = jParseString(json, "ip_servidor_distribuido");
	c.portDist = jParseInt(json, "porta_servidor_distribuido");
	c.name = jParseString(json, "nome");
	c.inputs = jParseGPIOs(json, "inputs", &c.inputSize);
	c.outputs = jParseGPIOs(json, "outputs", &c.outputSize);
	c.tSensors = jParseTempSensors(json, &c.sensorSize);
	c.counter = jParseInt(json, "counter");

	free(json);

	return c;
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

char *configToString(config c){
	cJSON *json = cJSON_CreateObject();

	cJSON *outputs = cJSON_CreateArray();	
	cJSON_AddItemToObject(json, "outputs", outputs);
	gpioToJson(c.outputs, c.outputSize, outputs);

	char *str = cJSON_PrintUnformatted(json);

	return str;
}

