#ifndef GPIO_H
#define GPIO_H

#include <wiringPi.h>
#include "jsonReader.h"
#include <string.h>
#include "dht22.h"
#include <signal.h>

void gpioInit(config *c);
void gpioRead(config *c);
void gpioWrite(config *c, int n);
void detectModifiedPin(config *c);
void detectCounter(config *c, int counterUp);
void OtherHandler();
void CounterHandler();
void interruptionsSetup(config *c);
#endif
