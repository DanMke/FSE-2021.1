#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wiringPi.h>

#include <signal.h>
#include <pthread.h>

#include <cJSON.h>
#include <../inc/dht22.h>

#define NUM_THREADS 3

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

//    sleep(2);

    exit(0);
}

void *thread_server(void *arg) {
    while (1) {
        sleep(1);
    }
}

void *thread_client(void *arg) {
    while (1) {
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
    pthread_create(&(threads[1]), NULL, thread_client, NULL);
    pthread_create(&(threads[2]), NULL, thread_server, NULL);

    while (1) {
        printf("Humidity = %.1f %% Temperature = %.1f *C\n", dht22.humidity, dht22.celsiusTemperature);
        sleep(1);
    }

    return 0;
}