#include "gpio.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

pthread_t threadUpdate;

config c; 
deviceGpio *commands;
int counterUp;
int firstTime=0;
int clientSocketG;

unsigned int commandSize;

void openSendSocket(){

	int clientSocket;
	struct sockaddr_in serverAddr;
	unsigned short serverPort;
	char *ipServer;

	ipServer = c.ipCentral;
	serverPort = c.portCentral;
	
	if((clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf("Falha no Socket\n");
		exit(1);
	}

	// Montar a estrutura sockaddr_in
	memset(&serverAddr, 0, sizeof(serverAddr)); // Zerando a estrutura de dados
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(ipServer);
	serverAddr.sin_port = htons(serverPort);

	while(connect(clientSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
		printf("Aguardando Servidor\n");
		sleep(1);
	}
	clientSocketG = clientSocket;
}


void sendHandler(char *msg) {
	int rec, sen;
	unsigned int size = strlen(msg) + 1;
	send(clientSocketG, &size, sizeof(unsigned int), 0);
	while(1){
		if((sen = send(clientSocketG, msg, size, 0)) != size){
			printf("Erro no envio - send()\n");
		}
		char *buffer = malloc(size+1); 
		if ((rec = recv(clientSocketG, buffer, size, 0)) < 0)
			printf("Erro no recv()\n");
		else{
			if(strcmp(buffer, msg) == 0){
				char m = 'o';
				send(clientSocketG, &m, 1, 0);
				break;
			}
			else{
				char m = 'a';
				send(clientSocketG, &m, 1, 0);
			}
		}
	}
	if(firstTime){
		// Recebe
		unsigned int receiveSize;
		recv(clientSocketG, &receiveSize, sizeof(unsigned int), 0);
		char *buffer = malloc(receiveSize); 
		char other = 'a';
		
		while(1) {
			// Recebe Comandos
			if ((rec = recv(clientSocketG, buffer, receiveSize, 0)) < 0)
				printf("Erro no recv()\n");
			usleep(1000);
			// Confirma Comandos
			if ((sen = send(clientSocketG, buffer, receiveSize, 0)) != rec){
				printf("Erro no envio - send2()\n");
			}
			if ((rec = recv(clientSocketG, &other, 3, 0)) <= 0)
				printf("Erro no recv()\n");
			if(other == 'o')
				break;
		}
		commands = readInstructions(buffer);
		free(buffer);
	}
}

void checkChanges() {
	for(int i=0;i<c.outputSize;i++){
		if(c.outputs[i].on != commands[i].on){
			gpioWrite(&c, commands[i].gpio);
			c.outputs[i].on = commands[i].on;
		}
	}
}

void update(int s){
	if(s == SIGUSR1){
		detectModifiedPin(&c);
	}
	else if(s == SIGUSR2){
		detectCounter(&c, counterUp);
	}
}

void setCount(deviceGpio *devices, int size){
	for(int i=0;i<size;i++){
		if(devices[i].type[0] == 'c'){
			counterUp = i;
			break;
		}
	}
}

void *updateTemps(void *args) {
	while(1) {
		config t = c;
		gpioRead(&t);
		sleep(1);
		char *buffer = configToString(t);
		sendHandler(buffer);
		checkChanges();
	}
}
int main(int argc, const char * argv[]) {
	if (argc != 2){
		printf("Uso: $./bin/bin <ArquivoDeConfig>\n");
		return 1;
	}

	c = readConfig(argv[1]);	
	setCount(c.inputs, c.inputSize);
	
	openSendSocket();
	gpioInit(&c);
	detectModifiedPin(&c);
	for(int i=0;i<c.sensorSize;i++){
		c.tSensors[i].data.temperature = 0.0;
		c.tSensors[i].data.humidity = 0.0;
	}
	gpioRead(&c);

	char *buffer = configToString(c);
	
	sendHandler(buffer);
	firstTime = 1;	

	commands = malloc(sizeof(deviceGpio) * c.outputSize);

	signal(SIGUSR1, update);
	signal(SIGUSR2, update);

	interruptionsSetup(&c);

	pthread_create(&threadUpdate, NULL, &updateTemps, NULL);	
	while(1){
		pause();
	}

	freeConfig(&c);

	return 0;
}


