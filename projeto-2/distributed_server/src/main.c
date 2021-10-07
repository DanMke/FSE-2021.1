#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wiringPi.h>

#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <cJSON.h>
#include "../inc/dht22.h"

#define NUM_THREADS 3

#define IP_CENTRAL_SERVER "192.168.0.53"
#define IP_DISTRIBUTED_SERVER "192.168.0.22"
#define PORT_CENTRAL_SERVER 10006;
#define PORT_DISTRIBUTED_SERVER_T 10106
#define PORT_DISTRIBUTED_SERVER_1 10206
int PORT_DISTRIBUTED_SERVER;

int servidorSocket;

typedef struct {
    char *type;
    char *tag;
    int gpioPin;
    int status;
} Item;

typedef struct {
    char *ip;
    int port;
    char *name;
    Item *outputs;
    Item *inputs;
} Floor;

int outputsSize;
int inputsSize;

DHT22 dht22 = { .celsiusTemperature = -1, .humidity = -1 };
uint8_t dht_pin;  // 28 Terreo e 29 1o Andar

pthread_t threads[NUM_THREADS];

Floor floorGlobal;

int pavement;

void finishResources() {

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(threads[i]);
    }
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    close(servidorSocket);

    for (int i = 0; i < outputsSize; i++) {
        free(floorGlobal.outputs[i].type);
        free(floorGlobal.outputs[i].tag);
    }

    for (int i = 0; i < inputsSize; i++) {
        free(floorGlobal.inputs[i].type);
        free(floorGlobal.inputs[i].tag);
    }

    free(floorGlobal.outputs);
    free(floorGlobal.inputs);
    free(floorGlobal.ip);
    free(floorGlobal.name);

    sleep(1);

    printf("Finished\n");

    exit(0);
}

cJSON *createJsonDataStatus() {
    cJSON *dataStatus = cJSON_CreateObject();

    cJSON_AddItemToObject(dataStatus, "request_type", cJSON_CreateString("DATA_STATUS"));
    cJSON_AddItemToObject(dataStatus, "pavement", cJSON_CreateNumber(pavement));
    cJSON_AddItemToObject(dataStatus, "temperature", cJSON_CreateNumber(dht22.celsiusTemperature));
    cJSON_AddItemToObject(dataStatus, "humidity", cJSON_CreateNumber(dht22.humidity));

    cJSON *outputs = cJSON_CreateArray();

    for (int i = 0; i < outputsSize; i++) {
        cJSON *output = cJSON_CreateObject();

        cJSON_AddItemToObject(output, "type", cJSON_CreateString(floorGlobal.outputs[i].type));
        cJSON_AddItemToObject(output, "tag", cJSON_CreateString(floorGlobal.outputs[i].tag));
        cJSON_AddItemToObject(output, "gpio", cJSON_CreateNumber(floorGlobal.outputs[i].gpioPin));
        cJSON_AddItemToObject(output, "status", cJSON_CreateNumber(floorGlobal.outputs[i].status));

        cJSON_AddItemToArray(outputs, output);
    }

    cJSON_AddItemToObject(dataStatus, "outputs", outputs);

    cJSON *inputs = cJSON_CreateArray();

    for (int i = 0; i < inputsSize; i++) {
        cJSON *input = cJSON_CreateObject();

        cJSON_AddItemToObject(input, "type", cJSON_CreateString(floorGlobal.inputs[i].type));
        cJSON_AddItemToObject(input, "tag", cJSON_CreateString(floorGlobal.inputs[i].tag));
        cJSON_AddItemToObject(input, "gpio", cJSON_CreateNumber(floorGlobal.inputs[i].gpioPin));
        cJSON_AddItemToObject(input, "status", cJSON_CreateNumber(floorGlobal.inputs[i].status));

        cJSON_AddItemToArray(inputs, input);
    }

    cJSON_AddItemToObject(dataStatus, "inputs", inputs);

    return dataStatus;
}

void TrataClienteTCP(int socketCliente) {
    char buffer[500];
    int tamanhoRecebido;

    if((tamanhoRecebido = recv(socketCliente, buffer, 500, 0)) < 0)
        printf("Erro no recv()\n");

    while (tamanhoRecebido > 0) {
        if(send(socketCliente, buffer, tamanhoRecebido, 0) != tamanhoRecebido)
            printf("Erro no envio - send()\n");

        if((tamanhoRecebido = recv(socketCliente, buffer, 500, 0)) < 0)
            printf("Erro no recv()\n");
    }
    cJSON *receiveData = cJSON_Parse(buffer);

    cJSON *request_type = cJSON_GetObjectItemCaseSensitive(receiveData, "request_type");

    if (strcmp(request_type->valuestring, "CHANGE_STATUS") == 0) {
        cJSON *gpioPin = cJSON_GetObjectItemCaseSensitive(receiveData, "gpioPin");
        int pin = gpioPin->valueint;
        cJSON *state = cJSON_GetObjectItemCaseSensitive(receiveData, "state");
        int desiredState = state->valueint;

        if (desiredState == 0) {
            digitalWrite(pin, LOW);
        } else {
            digitalWrite(pin, HIGH);
        }
    }

}

void *thread_server(void *arg) {
    int socketCliente;
    struct sockaddr_in servidorAddr;
    struct sockaddr_in clienteAddr;
    unsigned short servidorPorta;
    unsigned int clienteLength;

    servidorPorta = PORT_DISTRIBUTED_SERVER;

    // Abrir Socket
    if((servidorSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        printf("falha no socket do Servidor\n");

    // Montar a estrutura sockaddr_in
    memset(&servidorAddr, 0, sizeof(servidorAddr)); // Zerando a estrutura de dados
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servidorAddr.sin_port = htons(servidorPorta);

    // Bind
    if(bind(servidorSocket, (struct sockaddr *) &servidorAddr, sizeof(servidorAddr)) < 0)
        printf("Falha no Bind\n");

    // Listen
    if(listen(servidorSocket, 20) < 0)
        printf("Falha no Listen\n");

    while(1) {
        clienteLength = sizeof(clienteAddr);
        if((socketCliente = accept(servidorSocket,
                                   (struct sockaddr *) &clienteAddr,
                                   &clienteLength)) < 0)
            printf("Falha no Accept\n");

        printf("Conexão do Cliente %s\n", inet_ntoa(clienteAddr.sin_addr));

        TrataClienteTCP(socketCliente);
        close(socketCliente);
        sleep(1);

    }
}

void *thread_client(void *arg) {
    while (1) {
        int clienteSocket;
        struct sockaddr_in servidorAddr;
        unsigned short servidorPorta;
        char *IP_Servidor;
        unsigned int tamanhoMensagem;

        int bytesRecebidos;
        int totalBytesRecebidos;

        IP_Servidor = IP_CENTRAL_SERVER;
        servidorPorta = PORT_CENTRAL_SERVER;

        // Criar Socket
        if((clienteSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            printf("Erro no socket()\n");

        // Construir struct sockaddr_in
        memset(&servidorAddr, 0, sizeof(servidorAddr)); // Zerando a estrutura de dados
        servidorAddr.sin_family = AF_INET;
        servidorAddr.sin_addr.s_addr = inet_addr(IP_Servidor);
        servidorAddr.sin_port = htons(servidorPorta);

        // Connect
        if(connect(clienteSocket, (struct sockaddr *) &servidorAddr,
                   sizeof(servidorAddr)) < 0)
            printf("Erro no connect()\n");

        cJSON *dataStatus = createJsonDataStatus();

        char *parseDataStatus = cJSON_Print(dataStatus);
        tamanhoMensagem = strlen(parseDataStatus);

        char *buffer = malloc((tamanhoMensagem + 1) * sizeof(char));

//        printf("tamanho da mensagem: %d\n", tamanhoMensagem);

        if(send(clienteSocket, parseDataStatus, tamanhoMensagem, 0) != tamanhoMensagem)
            printf("Erro no envio: numero de bytes enviados diferente do esperado\n");

        totalBytesRecebidos = 0;
        while(totalBytesRecebidos < tamanhoMensagem) {
            if((bytesRecebidos = recv(clienteSocket, buffer, 1350-1, 0)) <= 0)
                printf("Não recebeu o total de bytes enviados\n");
            totalBytesRecebidos += bytesRecebidos;
            buffer[bytesRecebidos] = '\0';
//            printf("%s\n", buffer);
        }
        close(clienteSocket);
        free(buffer);

        sleep(1);
    }
}

void *thread_dht22(void *arg) {
    while (1) {
        read_dht_data(&dht22, dht_pin);
//        int no_success = read_dht_data(&dht22, dht_pin);
//        if (no_success) {
//            finishResources();
//        }
        sleep(1);
    }
}

char* readFile(char *filename) {
    FILE *fp;
    char *buffer;
    long numbytes;

    fp = fopen(filename, "r");

    if(fp == NULL) {
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    numbytes = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    buffer = (char*)calloc(numbytes, sizeof(char));

    if(buffer == NULL) {
        exit(1);
    }

    fread(buffer, sizeof(char), numbytes, fp);
    fclose(fp);

    return buffer;
}

Floor getJsonData(char *buffer) {
    Floor floor_;

    cJSON *fileData = cJSON_Parse(buffer);

    cJSON *ip = cJSON_GetObjectItemCaseSensitive(fileData, "ip");
    int ipStringLength = strlen(ip->valuestring);
    floor_.ip = malloc((ipStringLength + 1) * sizeof(char));
    strcpy(floor_.ip, ip->valuestring);

    cJSON *name = cJSON_GetObjectItemCaseSensitive(fileData, "nome");
    int nameStringLength = strlen(name->valuestring);
    floor_.name = malloc((nameStringLength + 1) * sizeof(char));
    strcpy(floor_.name, name->valuestring);

    cJSON *port = cJSON_GetObjectItemCaseSensitive(fileData, "porta");
    floor_.port = port->valueint;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(fileData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize = cJSON_GetArraySize(outputs);
    floor_.outputs = malloc((outputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");

        int typeStringLength = strlen(type->valuestring);
        floor_.outputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(floor_.outputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        floor_.outputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(floor_.outputs[counter].tag, tag->valuestring);

        floor_.outputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(fileData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize = cJSON_GetArraySize(inputs);
    floor_.inputs = malloc((inputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");

        int typeStringLength = strlen(type->valuestring);
        floor_.inputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(floor_.inputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        floor_.inputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(floor_.inputs[counter].tag, tag->valuestring);

        floor_.inputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    cJSON_Delete(fileData);

    return floor_;
}

int initWiringPi() {
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Failed to initialize wiringPi\n");
        return 1;
    }
    return 0;
}

int main (int argc, char *argv[]) {

    pavement = atoi(argv[1]);
    char *filename;

    if (pavement == 0) {
        filename = "configuracao_andar_terreo.json";
        PORT_DISTRIBUTED_SERVER = PORT_DISTRIBUTED_SERVER_T;
        dht_pin = 28;
    } else {
        filename = "configuracao_andar_1.json";
        PORT_DISTRIBUTED_SERVER = PORT_DISTRIBUTED_SERVER_1;
        dht_pin = 29;
    }

    printf("Floor: %d\nPort: %d\ndht_pin: %d\nFile: %s\n", pavement, PORT_DISTRIBUTED_SERVER, dht_pin, filename);

    char *buffer = readFile(filename);

    floorGlobal = getJsonData(buffer);

    free(buffer);

    if (initWiringPi() == 1) {
        return 1;
    }

    printf("IP: %s\n", floorGlobal.ip);
    printf("PORT: %d\n", floorGlobal.port);
    printf("NAME: %s\n", floorGlobal.name);

    printf("OUTPUTS:\n");
    for (int i = 0; i < outputsSize; i++) {
        printf("type: %s\n", floorGlobal.outputs[i].type);
        printf("tag: %s\n", floorGlobal.outputs[i].tag);
        printf("gpio: %d\n", floorGlobal.outputs[i].gpioPin);

        pinMode(floorGlobal.outputs[i].gpioPin, OUTPUT);
        floorGlobal.outputs[i].status = digitalRead(floorGlobal.outputs[i].gpioPin);
        printf("State: %d\n", floorGlobal.outputs[i].status);
//        digitalWrite(floorGlobal.outputs[i].gpioPin, HIGH);
    }

    printf("INPUTS:\n");
    for (int i = 0; i < inputsSize; i++) {
        printf("type: %s\n", floorGlobal.inputs[i].type);
        printf("tag: %s\n", floorGlobal.inputs[i].tag);
        printf("gpio: %d\n", floorGlobal.inputs[i].gpioPin);

        pinMode(floorGlobal.inputs[i].gpioPin, INPUT);
        floorGlobal.inputs[i].status = digitalRead(floorGlobal.inputs[i].gpioPin);
        printf("State: %d\n", floorGlobal.inputs[i].status);
    }

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_dht22, NULL);
    pthread_create(&(threads[1]), NULL, thread_server, NULL);
    pthread_create(&(threads[2]), NULL, thread_client, NULL);

    while (1) {
        printf("Humidity = %.1f %% Temperature = %.1f *C\n", dht22.humidity, dht22.celsiusTemperature);
        sleep(1);
    }

    return 0;
}