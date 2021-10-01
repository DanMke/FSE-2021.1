#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

typedef struct {
    char *type;
    char *tag;
    int gpioPin;
} Item;

typedef struct {
    char *ip;
    int port;
    Item *outputs;
    Item *inputs;
} Floor;

int outputsSize;
int inputsSize;

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

    floor.ip = malloc(strlen(ip->valuestring) * sizeof(char));
    strcpy(floor.ip, ip->valuestring);

    cJSON *port = cJSON_GetObjectItemCaseSensitive(fileData, "porta");
    floor.port = port->valueint;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(fileData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize = cJSON_GetArraySize(outputs);
    floor.outputs = malloc(outputsSize * sizeof(Item));

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");

        floor.outputs[counter].type = malloc(strlen(type->valuestring) * sizeof(char));
        strcpy(floor.outputs[counter].type, type->valuestring);

        floor.outputs[counter].tag = malloc(strlen(tag->valuestring) * sizeof(char));
        strcpy(floor.outputs[counter].tag, tag->valuestring);

        floor.outputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(fileData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize = cJSON_GetArraySize(inputs);
    floor.inputs = malloc(inputsSize * sizeof(Item));

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");

        floor.inputs[counter].type = malloc(strlen(type->valuestring) * sizeof(char));
        strcpy(floor.inputs[counter].type, type->valuestring);

        floor.inputs[counter].tag = malloc(strlen(tag->valuestring) * sizeof(char));
        strcpy(floor.inputs[counter].tag, tag->valuestring);

        floor.inputs[counter].gpioPin = gpio->valueint;

        counter++;
    }

    cJSON_Delete(fileData);

    return floor;
}

int main () {

    char *buffer = readFile("configuracao_andar_terreo.json");

    Floor floor = getJsonData(buffer);

    free(buffer);

    printf("IP: %s\n", floor.ip);
    printf("PORT: %d\n", floor.port);

    printf("OUTPUTS:\n");
    for (int i = 0; i < outputsSize; i++) {
        printf("type: %s\n", floor.outputs[i].type);
        printf("tag: %s\n", floor.outputs[i].tag);
        printf("gpio: %d\n", floor.outputs[i].gpioPin);
    }

    printf("INPUTS:\n");
    for (int i = 0; i < inputsSize; i++) {
        printf("type: %s\n", floor.inputs[i].type);
        printf("tag: %s\n", floor.inputs[i].tag);
        printf("gpio: %d\n", floor.inputs[i].gpioPin);
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

    return 0;
}