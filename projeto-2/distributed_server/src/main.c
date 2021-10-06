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
#define PORT_CENTRAL_SERVER 10006
#define PORT_DISTRIBUTED_SERVER_T 10106
#define PORT_DISTRIBUTED_SERVER_1 10206

int servidorSocket;

typedef struct {
    char *type;
    char *tag;
    int gpioPin;
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
uint8_t dht_pin = 28;  // 28 Terreo e 29 1o Andar

pthread_t threads[NUM_THREADS];

void finishResources() {
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(threads[i]);
    }
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    close(servidorSocket);

    sleep(1);

    printf("Finished\n");

    exit(0);
}

void TrataClienteTCP(int socketCliente) {
    char buffer[16];
    int tamanhoRecebido;

    if((tamanhoRecebido = recv(socketCliente, buffer, 16, 0)) < 0)
        printf("Erro no recv()\n");

    while (tamanhoRecebido > 0) {
        if(send(socketCliente, buffer, tamanhoRecebido, 0) != tamanhoRecebido)
            printf("Erro no envio - send()\n");

        if((tamanhoRecebido = recv(socketCliente, buffer, 16, 0)) < 0)
            printf("Erro no recv()\n");
    }
    printf("receive %s\n", buffer);

}

void *thread_server(void *arg) {
    int socketCliente;
    struct sockaddr_in servidorAddr;
    struct sockaddr_in clienteAddr;
    unsigned short servidorPorta;
    unsigned int clienteLength;

    servidorPorta = PORT_DISTRIBUTED_SERVER_T;

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
        char *mensagem;
        char buffer[16];
        unsigned int tamanhoMensagem;

        int bytesRecebidos;
        int totalBytesRecebidos;

        IP_Servidor = IP_CENTRAL_SERVER;
        servidorPorta = PORT_CENTRAL_SERVER;
        mensagem = "teste d to c";

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

        tamanhoMensagem = strlen(mensagem);

        if(send(clienteSocket, mensagem, tamanhoMensagem, 0) != tamanhoMensagem)
            printf("Erro no envio: numero de bytes enviados diferente do esperado\n");

        totalBytesRecebidos = 0;
        while(totalBytesRecebidos < tamanhoMensagem) {
            if((bytesRecebidos = recv(clienteSocket, buffer, 16-1, 0)) <= 0)
                printf("Não recebeu o total de bytes enviados\n");
            totalBytesRecebidos += bytesRecebidos;
            buffer[bytesRecebidos] = '\0';
            printf("%s\n", buffer);
        }
        close(clienteSocket);

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
    Floor floor;

    cJSON *fileData = cJSON_Parse(buffer);

    cJSON *ip = cJSON_GetObjectItemCaseSensitive(fileData, "ip");
    int ipStringLength = strlen(ip->valuestring);
    printf("teste: %d\n", ipStringLength);

    floor.ip = malloc((ipStringLength + 1) * sizeof(char));
    strcpy(floor.ip, ip->valuestring);
    printf("teste: %s\n", floor.ip);

    cJSON *name = cJSON_GetObjectItemCaseSensitive(fileData, "nome");
    int nameStringLength = strlen(name->valuestring);
    printf("teste: %d\n", nameStringLength);

    floor.name = malloc((nameStringLength + 1) * sizeof(char));
    strcpy(floor.name, name->valuestring);
    printf("teste: %s\n", floor.name);

    cJSON *port = cJSON_GetObjectItemCaseSensitive(fileData, "porta");
    floor.port = port->valueint;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(fileData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize = cJSON_GetArraySize(outputs);
    floor.outputs = malloc((outputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");

        int typeStringLength = strlen(type->valuestring);
        floor.outputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(floor.outputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        floor.outputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(floor.outputs[counter].tag, tag->valuestring);

        floor.outputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(fileData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize = cJSON_GetArraySize(inputs);
    floor.inputs = malloc((inputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");

        int typeStringLength = strlen(type->valuestring);
        floor.inputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(floor.inputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        floor.inputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(floor.inputs[counter].tag, tag->valuestring);

        floor.inputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    cJSON_Delete(fileData);

    return floor;
}

int initWiringPi() {
    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Failed to initialize wiringPi\n");
        return 1;
    }
    return 0;
}

int main (int argc, char *argv[]) {

    char *buffer = readFile("configuracao_andar_terreo.json");

    Floor floor = getJsonData(buffer);

    free(buffer);

    if (initWiringPi() == 1) {
        return 1;
    }

    printf("IP: %s\n", floor.ip);
    printf("PORT: %d\n", floor.port);
    printf("NAME: %s\n", floor.name);

    printf("OUTPUTS:\n");
    for (int i = 0; i < outputsSize; i++) {
        printf("type: %s\n", floor.outputs[i].type);
        printf("tag: %s\n", floor.outputs[i].tag);
        printf("gpio: %d\n", floor.outputs[i].gpioPin);

        pinMode(floor.outputs[i].gpioPin, OUTPUT);
        printf("State: %d\n", digitalRead(floor.outputs[i].gpioPin));
        digitalWrite(floor.outputs[i].gpioPin, HIGH);
        printf("State: %d\n", digitalRead(floor.outputs[i].gpioPin));
    }

    printf("INPUTS:\n");
    for (int i = 0; i < inputsSize; i++) {
        printf("type: %s\n", floor.inputs[i].type);
        printf("tag: %s\n", floor.inputs[i].tag);
        printf("gpio: %d\n", floor.inputs[i].gpioPin);

        pinMode(floor.inputs[i].gpioPin, INPUT);
        printf("State: %d\n", digitalRead(floor.inputs[i].gpioPin));
        digitalWrite(floor.inputs[i].gpioPin, HIGH);
        printf("State: %d\n", digitalRead(floor.inputs[i].gpioPin));
    }

    for (int i = 0; i < outputsSize; i++) {
        free(floor.outputs[i].type);
        free(floor.outputs[i].tag);
    }

    for (int i = 0; i < inputsSize; i++) {
        free(floor.inputs[i].type);
        free(floor.inputs[i].tag);
    }

    free(floor.outputs);
    free(floor.inputs);
    free(floor.ip);
    free(floor.name);

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_dht22, NULL);
    pthread_create(&(threads[1]), NULL, thread_server, NULL);
    sleep(5);
    pthread_create(&(threads[2]), NULL, thread_client, NULL);

    while (1) {
        printf("Humidity = %.1f %% Temperature = %.1f *C\n", dht22.humidity, dht22.celsiusTemperature);
        sleep(1);
    }

    return 0;
}