#include "../inc/uart_modbus.h"

#include "crc16.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>         //Used for UART
#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

int initialize_uart() {
    int uart0_filestream = -1;

    uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
    if (uart0_filestream == -1) {
        printf("Erro - Não foi possível iniciar a UART.\n");
        exit(1);
    } else {
        printf("UART inicializada!\n");
    }
    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;     //<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);

    return uart0_filestream;
}

void write_in_uart(int uart0_filestream, char device_code, char code, char subcode) {
    unsigned char tx_buffer[9];

    tx_buffer[0] = device_code;
    tx_buffer[1] = code;
    tx_buffer[2] = subcode;

    char university_registration[4] = {7, 0, 0, 3};

    memcpy(&tx_buffer[3], &university_registration, 4);

    short crc = calcula_CRC(tx_buffer, 7);

    memcpy(&tx_buffer[7], &crc, 2);

    printf("Buffers de memória criados!\n");

    if (uart0_filestream != -1) {
        printf("Escrevendo caracteres na UART ...");

        int count = write(uart0_filestream, tx_buffer, 9);
        if (count < 0) {
            printf("UART TX error\n");
        } else {
            printf("escrito.\n");
        }
    }
}

void read_data_uart(int uart0_filestream, unsigned char *rx_buffer) {
    if (uart0_filestream == -1) {
        exit(1);
    }
    // Read up to 255 characters from the port if they are there
    int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);      //Filestream, buffer to store in, number of bytes to read (max)
    if (rx_length < 0) {
        printf("Erro na leitura.\n"); //An error occured (will occur if there are no bytes)
        exit(1);
    } else if (rx_length == 0) {
        printf("Nenhum dado disponível.\n"); //No data waiting
        exit(1);
    } else {
        short crc = calcula_CRC(rx_buffer, rx_length - 2);
        short oldCrc;
        memcpy(&oldCrc, rx_buffer + (rx_length - 2), 2);

        if (crc != oldCrc) {
            close(uart0_filestream);
            exit(1);
        }

        //Bytes received
        rx_buffer[rx_length] = '\0';
    }
}

float request_internal_temperature(int uart0_filestream) {
    write_in_uart(uart0_filestream, 0x01, 0x23, 0xC1);

    sleep(2);

    unsigned char *rx_buffer;
    rx_buffer = (unsigned char*) malloc(9 * sizeof(unsigned char));
    read_data_uart(uart0_filestream, rx_buffer);

    float currentFloat;
    memcpy(&currentFloat, rx_buffer+3, 4);

    free(rx_buffer);

    return currentFloat;
}

float request_potentiometer_temperature(int uart0_filestream) {
    write_in_uart(uart0_filestream, 0x01, 0x23, 0xC2);

    sleep(2);

    unsigned char *rx_buffer;
    rx_buffer = (unsigned char*) malloc(9 * sizeof(unsigned char));
    read_data_uart(uart0_filestream, rx_buffer);

    float currentFloat;
    memcpy(&currentFloat, rx_buffer+3, 4);

    free(rx_buffer);

    return currentFloat;
}

int request_key_state(int uart0_filestream) {
    write_in_uart(uart0_filestream, 0x01, 0x23, 0xC3);

    sleep(2);

    unsigned char *rx_buffer;
    rx_buffer = (unsigned char*) malloc(9 * sizeof(unsigned char));
    read_data_uart(uart0_filestream, rx_buffer);

    int currentInteger;
    memcpy(&currentInteger, rx_buffer+3, 4);

    free(rx_buffer);

    return currentInteger;
}