#include <sys/socket.h>
#include <arpa/inet.h>
#include "jsonReader.h"
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h> 


pthread_t threadConnect;

int highlight = 0, highCount;
int alarmState = 0, alarmed = 0, fumes = -1;
int serverSocket;
int floors = 0;
int *clientSocketG;
config *c;
config *commands;
int *firstTime;

void openReceiveSocket(const char * argv[]){
	struct sockaddr_in serverAddr;
	unsigned int serverPort;

	serverPort = atoi(argv[2]);

	if((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		printf("Falha no socket do Servidor\n");

	// Montar a estrutura sockaddr_in
	memset(&serverAddr, 0, sizeof(serverAddr)); // Zerando a estrutura de dados
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(serverPort);

	// Bind
	if(bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
		printf("Falha no Bind\n");
}

int comparator(const void *a, const void *b) {
   return *(int*) a > *(int*) b;
}

int countFloors(int floor){
	int *count = malloc(sizeof(int)*floors);
	int *order = malloc(sizeof(int)*floors);
	for(int i=0; i<floors;i++){
		count[i] = c[floor].counter;
		order[i] = c[floor].counter;
	}
	
	qsort(order, floor, sizeof(int), comparator);
	int tmp;
	for(int i=0; i<floors;i++){
		if(count[i] == order[0])
			highCount = i;
		if(order[i] == order[floor]){
			tmp = order[floor];
			for(int j=floors-1;j>floor;j--)
				tmp -= order[j];
		}
	}
	free(count);
	free(order);
	return tmp;
}

int acceptConnection(){
	int clientSocket;
	unsigned int clientLength;
	struct sockaddr_in clientAddr;
	// Listen
	if(listen(serverSocket, 10) < 0)
		printf("Falha no Listen\n");	

	clientLength = sizeof(clientAddr);
	if((clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddr, &clientLength)) < 0)
		printf("Falha no Accept\n");

	return clientSocket;
}

char *ReceiveHandler(int floor) {
	unsigned int receiveSize;
	recv(clientSocketG[floor-1], &receiveSize, sizeof(unsigned int), 0);
	char *buffer = malloc(receiveSize);
	char other = 'a';
	int rec, sen;
	
	// Recebe Config
	while(1) {
		// Recebe Config
		if ((rec = recv(clientSocketG[floor-1], buffer, receiveSize, 0)) < 0)
			printf("Erro no recv()\n");
		// Confirma Config
		if ((sen = send(clientSocketG[floor-1], buffer, receiveSize, 0)) != rec)
			printf("Erro no envio - send()\n");
		if ((rec = recv(clientSocketG[floor-1], &other, 3, 0)) <= 0)
			printf("Erro no recv()\n");
		if(other == 'o')
			break;
	}

	// Manda Comandos(Calcula)
	if(firstTime[floor-1]){
		char *msg = configToString(commands[floor-1]);
		unsigned int changeSize = strlen(msg) + 1;
		//printf("CHANGE SIZE: %d\n", changeSize);
		send(clientSocketG[floor-1], &changeSize, sizeof(unsigned int), 0);
		// Manda Comandos
		while(1){
			// Manda Comandos
			if((sen = send(clientSocketG[floor-1], msg, changeSize, 0)) != changeSize)
				printf("Erro no envio - send()\n");
			char *tmp = malloc(changeSize); 
			// Confirma Comandos
			if ((rec = recv(clientSocketG[floor-1], tmp, changeSize, 0)) < 0)
				printf("Erro no recv()\n");
			else{
				if(strcmp(tmp, msg) == 0){
					char m = 'o';
					send(clientSocketG[floor-1], &m, 1, 0);
					break;
				}
				else{
					char m = 'a';
					send(clientSocketG[floor-1], &m, 1, 0);
				}
			}
		}
	}
	
	return buffer;
}

int checkAlarmState(){
	int flag = 0;
	for(int floor=0;floor<floors;floor++){
		for(int i=0;i<c[floor].inputSize;i++){
			if(c[floor].inputs[i].on && (c[floor].inputs[i].type[0] == 'j' || c[floor].inputs[i].type[0] == 'p')){
				flag = 1;
				break;
			}
		}
	}
	return flag;
}

void setAlarmState(){
	if(alarmState)
		alarmState = 0;
	else if(!checkAlarmState())
		alarmState = 1;
}

void printGPIO(deviceGpio *GPIOs, int size, int flag){
	for(int i=0, j=0;i<size;i++){
		if(flag)
			printw("%d) ", j++);
		if(GPIOs[i].type[0] != 'c')
			printw("%s -> %s\n", GPIOs[i].tag, GPIOs[i].on == 1 ? "Ativado" : "Desativado");
	}
}

void printSensors(tempSensor *GPIOs, int size){
	for(int i=0;i<size;i++){
		printw("%s -> Temperatura: %f, Humidade: %f\n", GPIOs[i].sensor.tag, GPIOs[i].data.temperature, GPIOs[i].data.humidity);
	}
}

void setAll(int floor, int charge, char *type){
	for(int i=0;i<c[floor].outputSize;i++){
		if((strcmp(type, "all") == 0 || strcmp(type, c[floor].outputs[i].type) == 0) && strcmp("aspersor", c[floor].outputs[i].type) != 0){
			//c[floor].outputs[i].on = charge;
			commands[floor].outputs[i].on = charge;
		}
	}
}
void printAll(){
	clear();
	printw("Total de andares: %d\n", floors);
	printw("Alarme: %s\n", alarmState == 1 ? "Ativado" : "Desativado");
	if(alarmed)
		printw("Alarme Disparado! Desligue para parar\n");
	if(fumes != -1)
		printw("Fumaça detectada!\n");
	printw("===== %s =====\n", c[highlight].name);
	printw("-----INPUTS-----\n");
	printGPIO(c[highlight].inputs, c[highlight].inputSize, 0);
	printw("-----OUTPUTS-----\n");
	printGPIO(c[highlight].outputs, c[highlight].outputSize, 1);
	printw("-----Sensors-----\n");
	printSensors(c[highlight].tSensors, c[highlight].sensorSize);
	//printw("Pessoas: %d\n", countFloors(highlight));
	//printw("Pessoas Sensor: %d\n", c[highlight].counter);
	//printw("Pessoas Total: %d\n", c[highCount].counter);
	printw("Pressione C para alternar entre andares\n");
	printw("Pressione o número do dispositivo para alternar os estado(Exceto Aspersor)\n");
	printw("Pressione A e B para desligar e ligar tudo, ");
	printw("pressione D e E para desligar e ligar lampadas\n");
	printw("Pressione F para desligar ar-condicionados e G para ligar\n");
	printw("Pressione H para alternar o estado do Alarme\n");
	refresh();
}

void playAlarm(){
	alarmed = 1;
}

void fumesHandler(int charge){
	for(int floor=0;floor<floors;floor++){
		for(int i=0;i<c[floor].outputSize;i++){
			if(c[floor].outputs[i].type[1] == 's'){
				commands[floor].outputs[i].on = charge;
			}
		}
	}
}

void *update(void *args) {
	int floor = floors;
	while(1) {
		char *buffer = ReceiveHandler(floor);
		config bak = readConfig(buffer);
		if(bak.name != NULL){
			c[floor-1] = bak;
		}
		if(alarmState && checkAlarmState()){
			playAlarm();
		}

		for(int i=0;i<c[floor-1].inputSize;i++){
			if(c[floor-1].inputs[i].type[0] == 'f'){
				if(c[floor-1].inputs[i].on){
					fumes = floor;
					fumesHandler(1);
				}
				else if(fumes == floor || fumes == -1){
					fumes = -1;
					fumesHandler(0);
				}
			}
		}
	}
}

void *listener(void *args) {
	clientSocketG = malloc(1);
	c = malloc(1);
	firstTime = malloc(1);
	while(1){
		clientSocketG = realloc(clientSocketG, (floors+1)*sizeof(int));
		clientSocketG[floors] = acceptConnection(args);
		c = realloc(c, (floors+1) * sizeof(config));
		commands = realloc(commands, (floors+1) * sizeof(config));
		firstTime = realloc(firstTime, (floors+1) * sizeof(int));
		firstTime[floors] = 0;
		char *buffer = ReceiveHandler(floors+1);
		firstTime[floors] = 1;
		c[floors] = readConfig(buffer);
		commands[floors] = c[floors];

		floors += 1;
		pthread_t threadUpdate;
		pthread_create(&threadUpdate, NULL, &update, NULL);
	}
}

int main(int argc, const char * argv[]){
	
	if (argc != 3){
		printf("Uso: $./bin/bin <IP> <Porta>\n");
		return 1;
	}

	openReceiveSocket(argv);
	pthread_create(&threadConnect, NULL, &listener, NULL);	
	while(!floors)
		sleep(1);
	
	//while(1);
	initscr();
	timeout(1000);
	while(1){
		char ch = 126;
		printAll();
		refresh();
		ch = getch();
		refresh();
		if(ch-48 < c[highlight].outputSize && ch-48 >= 0){
			if(commands[highlight].outputs[(int) ch-48].type[1] != 's')
				commands[highlight].outputs[(int) ch-48].on = !commands[highlight].outputs[(int) ch-48].on;
		}
		// Desliga predio
		else if (ch == 'a')
			for(int i=0;i<floors;i++)
				setAll(i, 0, "all");
		// Liga predio
		else if (ch == 'b')
			for(int i=0;i<floors;i++)
				setAll(i, 1, "all");
		// Alterna andar
		else if (ch == 'c')
			highlight = (highlight + 1) % floors;
		// Desliga lampadas
		else if (ch == 'd')
			setAll(highlight, 0, "lampada");
		// Liga lampadas
		else if (ch == 'e')
			setAll(highlight, 1, "lampada");	
		// Desliga ar-condicionados
		else if (ch == 'f')
			setAll(highlight, 0, "ar-condicionado");
		// Liga ar-condicionados
		else if (ch == 'g')
			setAll(highlight, 1, "ar-condicionado");
		// Alterna alarme
		else if (ch == 'h'){
			if(alarmed)
				alarmed = 0;
			setAlarmState();
		}
	}	
	pthread_join(threadConnect, NULL);
  	endwin();	

}

