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
    sleep(5);
    pthread_create(&(threads[1]), NULL, thread_client, NULL);

    while (1) {
//        printf("Teste\n");
        sleep(1);
    }

    return 0;
}