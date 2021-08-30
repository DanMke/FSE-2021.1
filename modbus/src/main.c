#include <stdio.h>
#include <string.h>
#include <unistd.h>         //Used for UART
#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

int main(int argc, const char * argv[]) {

    int uart0_filestream = -1;

    uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
    if (uart0_filestream == -1) {
        printf("Erro - Não foi possível iniciar a UART.\n");
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

    unsigned char tx_buffer[9];

    char matricula[4] = {7, 0, 0, 3};

    int option;
    printf("\nEnter the option choose:\n");
    printf("\nOption 161: ENVIA INT\n");
    printf("\nOption 162: ENVIA FLOAT\n");
    printf("\nOption 163: ENVIA STRING\n");
    printf("\nOption 1611: SOLICITA INT A\n");
    printf("\nOption 1621: SOLICITA FLOAT A\n");
    printf("\nOption 1631: SOLICITA STRING A\n\n");
    scanf("%d", &option);

    if (option == 161) {
        tx_buffer[0] = 0xB1;
        int data = 8521;

        memcpy(&tx_buffer[1], &data, 4);
        memcpy(&tx_buffer[5], &matricula, 4);
    } else if (option == 162) {
        tx_buffer[0] = 0xB2;
        float data = 8521;

        memcpy(&tx_buffer[1], &data, 4);
        memcpy(&tx_buffer[5], &matricula, 4);
    } else if (option == 163) {
        tx_buffer[0] = 0xB3;
        tx_buffer[1] = 3;
        char data[3] = {'H', 'd', '\0'};

        memcpy(&tx_buffer[2], &data, 3);
        memcpy(&tx_buffer[5], &matricula, 4);
    } else if (option == 1611) {

        tx_buffer[0] = 0xA1;
        memcpy(&tx_buffer[1], &matricula, 4);
    } else if (option == 1621) {

        tx_buffer[0] = 0xA2;
        memcpy(&tx_buffer[1], &matricula, 4);
    } else if (option == 1631) {

        tx_buffer[0] = 0xA3;
        memcpy(&tx_buffer[1], &matricula, 4);
    } else {
        printf("\nOption not exists\n");
        close(uart0_filestream);
        return 0;
    }

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

    sleep(2);

    //----- CHECK FOR ANY RX BYTES -----
    if (uart0_filestream != -1) {
        // Read up to 255 characters from the port if they are there
        unsigned char rx_buffer[9];
        int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);      //Filestream, buffer to store in, number of bytes to read (max)
        if (rx_length < 0) {
            printf("Erro na leitura.\n"); //An error occured (will occur if there are no bytes)
        } else if (rx_length == 0) {
            printf("Nenhum dado disponível.\n"); //No data waiting
        } else {
            //Bytes received
            rx_buffer[rx_length] = '\0';

            if (option == 161 || option == 1611) {
                int currentInteger;
                memcpy(&currentInteger, rx_buffer, rx_length);
                printf("INT %d: %d\n", rx_length, currentInteger);
                printf("%i Bytes lidos : %d\n", rx_length, currentInteger);
            } else if (option == 162 || option == 1621) {
                float currentFloat;
                memcpy(&currentFloat, rx_buffer, rx_length);
                printf("FLOAT %d: %f\n", rx_length, currentFloat);
                printf("%i Bytes lidos : %f\n", rx_length, currentFloat);
            } else {
                char currentString[255];
                memcpy(currentString, rx_buffer, rx_length);
                printf("STRING: %s\n", currentString);
                printf("%i Bytes lidos : %s\n", rx_length, currentString);
            }
        }
    }

    close(uart0_filestream);
    return 0;
}
