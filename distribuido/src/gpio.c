#include "gpio.h"

void gpioInit(config *c){
	wiringPiSetup();
	for(int i=0;i<c->inputSize;i++){
		pinMode(c->inputs[i].gpio, INPUT);
		c->inputs[i].on = digitalRead(c->inputs[i].gpio);
	}
	for(int i=0;i<c->outputSize;i++){
		pinMode(c->outputs[i].gpio, OUTPUT);
		c->outputs[i].on = digitalRead(c->outputs[i].gpio);
	}
}

void gpioRead(config *c){
	for(int i=0;i<c->sensorSize;i++){
		dhtData data = read_dht_data(c->tSensors[i].sensor.gpio);
		if(data.temperature != -1 && data.humidity != -1)
			c->tSensors[i].data = data;
	}
}

void gpioWrite(config *c, int n){
	int x = digitalRead(c->outputs[n].gpio);
	digitalWrite(c->outputs[n].gpio, !x);
	c->outputs[n].on = !x;
}

void detectModifiedPin(config *c){
	for(int i=0;i<c->inputSize;i++){
		if(c->inputs[i].type[0] != 'c')
			c->inputs[i].on = digitalRead(c->inputs[i].gpio);
	}
}

void detectCounter(config *c, int counterUp){
	if(digitalRead(c->inputs[counterUp].gpio)){
		c->counter++;
	}
	else{
		c->counter -= c->counter == 0 ? 0 : 1;
	}
} 

void OtherHandler(){
	raise(SIGUSR1);
}

void CounterHandler(){
	raise(SIGUSR2);
}

void interruptionsSetup(config *c){
	for(int i=0;i<c->inputSize;i++){
		if(c->inputs[i].type[0] == 'c')
			wiringPiISR(c->inputs[i].gpio, INT_EDGE_RISING, CounterHandler);
		else
			wiringPiISR(c->inputs[i].gpio, INT_EDGE_BOTH, OtherHandler);
	}
}

