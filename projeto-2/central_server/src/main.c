#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <ncurses.h>

#include <cJSON.h>

#define NUM_THREADS 2

#define IP_CENTRAL_SERVER "192.168.0.53"
#define IP_DISTRIBUTED_SERVER "192.168.0.22"
#define PORT_CENTRAL_SERVER 10006
#define PORT_DISTRIBUTED_SERVER_T 10106
#define PORT_DISTRIBUTED_SERVER_1 10206

#define WIDTH 100
#define HEIGHT 25

int startx = 0;
int starty = 0;

int end = 0;

pthread_t threads[NUM_THREADS];

int servidorSocket;

typedef struct {
    char *type;
    char *tag;
    int gpioPin;
    int status;
} Item;

typedef struct {
    int online;
    float temperature;
    float humidity;
    Item *outputs;
    Item *inputs;
} DataStatus;

DataStatus dataStatusGlobal_T = { .online = 0 };
DataStatus dataStatusGlobal_1 = { .online = 0 };

int outputsSize_T;
int inputsSize_T;

int outputsSize_1;
int inputsSize_1;

WINDOW *menu_win_t;
WINDOW *menu_win_1;


void finishResources() {

    clrtoeol();
    werase(menu_win_t);
    refresh();
    endwin();
    erase();

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

void parseDataT(cJSON *receiveData, char *buffer) {

    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(receiveData, "temperature");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(receiveData, "humidity");
    dataStatusGlobal_T.temperature = temperature->valuedouble;
    dataStatusGlobal_T.humidity = humidity->valuedouble;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(receiveData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize_T = cJSON_GetArraySize(outputs);
    if (dataStatusGlobal_T.online == 0) {
        dataStatusGlobal_T.outputs = malloc((outputsSize_T + 1) * sizeof(Item));
    }

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(output, "status");

        int typeStringLength = strlen(type->valuestring);
        if (dataStatusGlobal_T.online == 0) {
            dataStatusGlobal_T.outputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_T.outputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        if (dataStatusGlobal_T.online == 0) {
            dataStatusGlobal_T.outputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_T.outputs[counter].tag, tag->valuestring);

        dataStatusGlobal_T.outputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal_T.outputs[counter].gpioPin = status->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(receiveData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize_T = cJSON_GetArraySize(inputs);
    if (dataStatusGlobal_T.online == 0) {
        dataStatusGlobal_T.inputs = malloc((inputsSize_T + 1) * sizeof(Item));
    }

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(input, "status");

        int typeStringLength = strlen(type->valuestring);
        if (dataStatusGlobal_T.online == 0) {
            dataStatusGlobal_T.inputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_T.inputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        if (dataStatusGlobal_T.online == 0) {
            dataStatusGlobal_T.inputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_T.inputs[counter].tag, tag->valuestring);

        dataStatusGlobal_T.inputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal_T.inputs[counter].gpioPin = status->valueint;

        counter++;
    }
    dataStatusGlobal_T.online = 1;
}

void parseData1(cJSON *receiveData, char *buffer) {

    cJSON *temperature = cJSON_GetObjectItemCaseSensitive(receiveData, "temperature");
    cJSON *humidity = cJSON_GetObjectItemCaseSensitive(receiveData, "humidity");
    dataStatusGlobal_1.temperature = temperature->valuedouble;
    dataStatusGlobal_1.humidity = humidity->valuedouble;

    const cJSON *outputs = cJSON_GetObjectItemCaseSensitive(receiveData, "outputs");
    const cJSON *output = NULL;

    int counter = 0;
    outputsSize_1 = cJSON_GetArraySize(outputs);
    if (dataStatusGlobal_1.online == 0) {
        dataStatusGlobal_1.outputs = malloc((outputsSize_1 + 1) * sizeof(Item));
    }

    cJSON_ArrayForEach(output, outputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(output, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(output, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(output, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(output, "status");

        int typeStringLength = strlen(type->valuestring);
        if (dataStatusGlobal_1.online == 0) {
            dataStatusGlobal_1.outputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_1.outputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        if (dataStatusGlobal_1.online == 0) {
            dataStatusGlobal_1.outputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_1.outputs[counter].tag, tag->valuestring);

        dataStatusGlobal_1.outputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal_1.outputs[counter].gpioPin = status->valueint;

        counter++;
    }

    const cJSON *inputs = cJSON_GetObjectItemCaseSensitive(receiveData, "inputs");
    const cJSON *input = NULL;

    counter = 0;
    inputsSize_1 = cJSON_GetArraySize(inputs);
    if (dataStatusGlobal_1.online == 0) {
        dataStatusGlobal_1.inputs = malloc((inputsSize_1 + 1) * sizeof(Item));
    }

    cJSON_ArrayForEach(input, inputs) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(input, "type");
        cJSON *tag = cJSON_GetObjectItemCaseSensitive(input, "tag");
        cJSON *gpio = cJSON_GetObjectItemCaseSensitive(input, "gpio");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(input, "status");

        int typeStringLength = strlen(type->valuestring);
        if (dataStatusGlobal_1.online == 0) {
            dataStatusGlobal_1.inputs[counter].type = malloc((typeStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_1.inputs[counter].type, type->valuestring);

        int tagStringLength = strlen(tag->valuestring);
        if (dataStatusGlobal_1.online == 0) {
            dataStatusGlobal_1.inputs[counter].tag = malloc((tagStringLength + 1) * sizeof(char));
        }
        strcpy(dataStatusGlobal_1.inputs[counter].tag, tag->valuestring);

        dataStatusGlobal_1.inputs[counter].gpioPin = gpio->valueint;
        dataStatusGlobal_1.inputs[counter].gpioPin = status->valueint;

        counter++;
    }

//    printf("temperature: %.1lf\n", dataStatusGlobal_1.temperature);
//    printf("humidity: %.1lf\n", dataStatusGlobal_1.humidity);
//
//    for (int i = 0; i < outputsSize_1; i++) {
//        printf("type: %s\n", dataStatusGlobal_1.outputs[i].type);
//        printf("tag: %s\n", dataStatusGlobal_1.outputs[i].tag);
//        printf("gpio: %d\n", dataStatusGlobal_1.outputs[i].gpioPin);
//        printf("status: %d\n", dataStatusGlobal_1.outputs[i].status);
//    }
//
//    for (int i = 0; i < inputsSize_1; i++) {
//        printf("type: %s\n", dataStatusGlobal_1.inputs[i].type);
//        printf("tag: %s\n", dataStatusGlobal_1.inputs[i].tag);
//        printf("gpio: %d\n", dataStatusGlobal_1.inputs[i].gpioPin);
//        printf("status: %d\n", dataStatusGlobal_1.inputs[i].status);
//    }
    dataStatusGlobal_1.online = 1;
    // atualiza
    // dps free
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

    cJSON *receiveData = cJSON_Parse(buffer);

    cJSON *request_type = cJSON_GetObjectItemCaseSensitive(receiveData, "request_type");

    if (strcmp(request_type->valuestring, "DATA_STATUS") == 0) {
        cJSON *pavement = cJSON_GetObjectItemCaseSensitive(receiveData, "pavement");
        if (pavement->valueint == 0) {
            parseDataT(receiveData, buffer);
        } else if (pavement->valueint == 1) {
            parseData1(receiveData, buffer);
        }
    }

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

        TrataClienteTCP(socketCliente);
        close(socketCliente);
        sleep(1);

    }
}

cJSON *parseSendData(int gpioPin, int state) {
    cJSON *sendData = cJSON_CreateObject();

    cJSON_AddItemToObject(sendData, "request_type", cJSON_CreateString("CHANGE_STATUS"));
    cJSON_AddItemToObject(sendData, "gpioPin", cJSON_CreateNumber(gpioPin));
    cJSON_AddItemToObject(sendData, "state", cJSON_CreateNumber(-state));

    return sendData;
}

void send_data_socket(cJSON *sendData) {
    int clienteSocket;
    struct sockaddr_in servidorAddr;
    unsigned short servidorPorta;
    char *IP_Servidor;
    char buffer[500];
    unsigned int tamanhoMensagem;

    int bytesRecebidos;
    int totalBytesRecebidos;

    IP_Servidor = IP_DISTRIBUTED_SERVER;
    servidorPorta = PORT_DISTRIBUTED_SERVER_T;

    char *mensagem = cJSON_Print(sendData);
    tamanhoMensagem = strlen(mensagem);

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
        if((bytesRecebidos = recv(clienteSocket, buffer, 500-1, 0)) <= 0)
            printf("Não recebeu o total de bytes enviados\n");
        totalBytesRecebidos += bytesRecebidos;
        buffer[bytesRecebidos] = '\0';
        printf("%s\n", buffer);
    }
    close(clienteSocket);

}

void send_data(int gpioPin, int floor) {
    int actualState;
    if (floor == 0) {
        for (int i = 0; i < outputsSize_T; i++) {
            if (dataStatusGlobal_T.outputs[i].gpioPin == gpioPin) {
                actualState = dataStatusGlobal_T.outputs[i].status;
            }
        }
    } else {
        for (int i = 0; i < outputsSize_1; i++) {
            if (dataStatusGlobal_1.outputs[i].gpioPin == gpioPin) {
                actualState = dataStatusGlobal_1.outputs[i].status;
            }
        }
    }

    send_data_socket(parseSendData(gpioPin, actualState));
}

void print_menu_1(WINDOW *menu_win) {
    int x = 2, y = 2;
    box(menu_win, 0, 0);

    mvwprintw(menu_win, x, y, "Aperte a determinada tecla para alterar o estado: (22)=a / (25)=s / (8)=d / (12)=f");
    y+=2;
    mvwprintw(menu_win, y, x, "1 Andar");
    y+=2;

    mvwprintw(menu_win, y, x, "Temperatura: %.1lf Graus C", dataStatusGlobal_1.temperature);
    y++;
    mvwprintw(menu_win, y, x, "Humidade: %.1lf %%", dataStatusGlobal_1.humidity);
    y++;

    for (int i = 0; i < outputsSize_1; i++) {
        mvwprintw(menu_win, y, x, "(%d) %s -> %d", dataStatusGlobal_1.outputs[i].gpioPin, dataStatusGlobal_1.outputs[i].tag, dataStatusGlobal_1.outputs[i].status);
        y++;
    }

    for (int i = 0; i < inputsSize_1; i++) {
        mvwprintw(menu_win, y, x, "%s -> %d", dataStatusGlobal_1.inputs[i].tag, dataStatusGlobal_1.inputs[i].status);
        y++;
    }

    wrefresh(menu_win);
}

void print_menu_t(WINDOW *menu_win) {
    int x = 2, y = 2;
    box(menu_win, 0, 0);

    mvwprintw(menu_win, x, y, "Aperte a determinada tecla para alterar o estado: (4)=q / (17)=w / (27)=e / (7)=r / (16)=t");
    y+=2;
    mvwprintw(menu_win, y, x, "Terreo");
    y+=2;

    mvwprintw(menu_win, y, x, "Temperatura: %.1lf Graus C", dataStatusGlobal_T.temperature);
    y++;
    mvwprintw(menu_win, y, x, "Humidade: %.1lf %%", dataStatusGlobal_T.humidity);
    y++;

    for (int i = 0; i < outputsSize_T; i++) {
        mvwprintw(menu_win, y, x, "(%d) %s -> %d", dataStatusGlobal_T.outputs[i].gpioPin, dataStatusGlobal_T.outputs[i].tag, dataStatusGlobal_T.outputs[i].status);
        y++;
    }

    for (int i = 0; i < inputsSize_T; i++) {
        mvwprintw(menu_win, y, x, "%s -> %d", dataStatusGlobal_T.inputs[i].tag, dataStatusGlobal_T.inputs[i].status);
        y++;
    }

    wrefresh(menu_win);
}

void *thread_ncurses(void *arg) {

    initscr();
    clear();
    noecho();
    cbreak();

    int c;

    menu_win_t = newwin(HEIGHT, WIDTH, 2, 2);

    keypad(menu_win_t, TRUE);
    wtimeout(menu_win_t, 1);

    menu_win_1 = newwin(HEIGHT, WIDTH, 2 + HEIGHT, 2);

    keypad(menu_win_1, TRUE);
    wtimeout(menu_win_1, 1);

    mvprintw(0, 0, "CTRL+C ou CTRL+Z para encerrar o programa");

    refresh();

//    print_menu(menu_win_t);

    while(1) {
        c = wgetch(menu_win_t);

//        q = 113 / w = 119 / e = 101 / r = 114 / t = 116
//        a = 97 / s = 115 / d = 100 / f = 102
        switch(c) {

            case 113:
                send_data(4, 0);
                break;
            case 119:
                send_data(17, 0);
                break;
            case 101:
                send_data(27, 0);
                break;
            case 114:
                send_data(7, 0);
                break;
            case 116:
                send_data(16, 0);
                break;
            case 97:
                send_data(22, 1);
                break;
            case 115:
                send_data(25, 1);
                break;
            case 100:
                send_data(8, 1);
                break;
            case 102:
                send_data(12, 1);
                break;

            case KEY_BACKSPACE:
                clrtoeol();
                werase(menu_win_t);
                refresh();
                endwin();
                erase();
                sleep(2);
                end = 1;
                break;
        }

        werase(menu_win_t);
        print_menu_t(menu_win_t);
        werase(menu_win_1);
        print_menu_1(menu_win_1);
        refresh();
        sleep(1);
    }
}

int main (int argc, char *argv[]) {

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_server, NULL);
//    sleep(5);
//    pthread_create(&(threads[1]), NULL, thread_client, NULL);
    pthread_create(&(threads[1]), NULL, thread_ncurses, NULL);


    while (!end) {
//        printf("Teste\n");
        sleep(1);
    }

    finishResources();

    return 0;
}