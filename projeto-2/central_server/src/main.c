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

pthread_t threads[NUM_THREADS];

void finishResources() {
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_cancel(threads[i]);
    }
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    sleep(1);

    printf("Finished\n");

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

int main (int argc, char *argv[]) {

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_server, NULL);
    pthread_create(&(threads[1]), NULL, thread_client, NULL);

    while (1) {
        printf("Teste\n");
        sleep(1);
    }

    return 0;
}