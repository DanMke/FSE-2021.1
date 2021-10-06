#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <cJSON.h>

#define NUM_THREADS 2

#define IP_CENTRAL_SERVER "192.168.0.53"
#define IP_DISTRIBUTED_SERVER "192.168.0.22"
#define PORT_CENTRAL_SERVER 10006
#define PORT_DISTRIBUTED_SERVER_T 10106
#define PORT_DISTRIBUTED_SERVER_1 10206

pthread_t threads[NUM_THREADS];

int servidorSocket;

typedef struct {
    char *type;
    char *tag;
    int gpioPin;
    int status;
} Item;

typedef struct {
    char *request_type;
    float temperature;
    float humidity;
    Item *outputs;
    Item *inputs;
} DataStatus;

DataStatus dataStatusGlobal;

int outputsSize;
int inputsSize;

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

void parseData(char *buffer) {
    cJSON *receiveData = cJSON_Parse(buffer);

    cJSON *request_type = cJSON_GetObjectItemCaseSensitive(receiveData, "request_type");
    int requestTypeStringLength = strlen(request_type->valuestring);
    dataStatusGlobal.request_type = malloc((requestTypeStringLength + 1) * sizeof(char));
    strcpy(dataStatusGlobal.request_type, request_type->valuestring);

    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(receiveData, "temperature");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(receiveData, "humidity");
    dataStatusGlobal.temperature = temperature->valuedouble;
    dataStatusGlobal.humidity = humidity->valuedouble;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(receiveData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize = cJSON_GetArraySize(outputs);
    dataStatusGlobal.outputs = malloc((outputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(output, "status");

        int typeStringLength = strlen(type->valuestring);
        dataStatusGlobal.outputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(dataStatusGlobal.outputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        dataStatusGlobal.outputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(dataStatusGlobal.outputs[counter].tag, tag->valuestring);

        dataStatusGlobal.outputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal.outputs[counter].gpioPin = status->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(receiveData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize = cJSON_GetArraySize(inputs);
    dataStatusGlobal.inputs = malloc((inputsSize + 1) * sizeof(Item));

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(input, "status");

        int typeStringLength = strlen(type->valuestring);
        dataStatusGlobal.inputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        strcpy(dataStatusGlobal.inputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        dataStatusGlobal.inputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        strcpy(dataStatusGlobal.inputs[counter].tag, tag->valuestring);

        dataStatusGlobal.inputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal.inputs[counter].gpioPin = status->valueint;

        counter++;
    }

    printf("request_type: %s\n", dataStatusGlobal.request_type);
    printf("temperature: %.1lf\n", dataStatusGlobal.temperature);
    printf("humidity: %.1lf\n", dataStatusGlobal.humidity);

    for (int i = 0; i < outputsSize; i++) {
        printf("type: %s\n", dataStatusGlobal.outputs[i].type);
        printf("tag: %s\n", dataStatusGlobal.outputs[i].tag);
        printf("gpio: %d\n", dataStatusGlobal.outputs[i].gpioPin);
        printf("status: %d\n", dataStatusGlobal.outputs[i].status);
    }

    for (int i = 0; i < inputsSize; i++) {
        printf("type: %s\n", dataStatusGlobal.inputs[i].type);
        printf("tag: %s\n", dataStatusGlobal.inputs[i].tag);
        printf("gpio: %d\n", dataStatusGlobal.inputs[i].gpioPin);
        printf("status: %d\n", dataStatusGlobal.inputs[i].status);
    }
}

void TrataClienteTCP(int socketCliente) {
    char buffer[1350];
    int tamanhoRecebido;

    if((tamanhoRecebido = recv(socketCliente, buffer, 1350, 0)) < 0)
        printf("Erro no recv()\n");

    while (tamanhoRecebido > 0) {
        if(send(socketCliente, buffer, tamanhoRecebido, 0) != tamanhoRecebido)
            printf("Erro no envio - send()\n");

        if((tamanhoRecebido = recv(socketCliente, buffer, 1350, 0)) < 0)
            printf("Erro no recv()\n");
    }

    parseData(buffer);
}

void *thread_server(void *arg) {
    int socketCliente;
    struct sockaddr_in servidorAddr;
    struct sockaddr_in clienteAddr;
    unsigned short servidorPorta;
    unsigned int clienteLength;

    servidorPorta = PORT_CENTRAL_SERVER;

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

        IP_Servidor = IP_DISTRIBUTED_SERVER;
        servidorPorta = PORT_DISTRIBUTED_SERVER_T;
        mensagem = "teste c to d";
        printf("teste %s\n", IP_DISTRIBUTED_SERVER);
        printf("teste %d\n", PORT_DISTRIBUTED_SERVER_T);

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

int main (int argc, char *argv[]) {

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_server, NULL);
//    sleep(5);
//    pthread_create(&(threads[1]), NULL, thread_client, NULL);

    while (1) {
//        printf("Teste\n");
        sleep(1);
    }

    return 0;
}