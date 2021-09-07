#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <wiringPiI2C.h>
#include <wiringPi.h>

#include <fcntl.h>
#include <termios.h>

#include <errno.h>

#include <signal.h>

#include <pthread.h>

#include <time.h>

#include "bme280.h"
#include "i2clcd.h"
#include "crc16.h"
#include "uart_modbus.h"
#include "gpio.h"
#include "pid.h"

void finishResources();

void *thread_bme280(void *arg);
void *thread_uart(void *arg);
void *thread_control(void *arg);
void *thread_csv(void *arg);

void create_csv();
void add_data_in_csv();

void menu();

pthread_t threads[4];
int fd;
int uart0_filestream;
struct bme280_dev dev;
struct bme280_data comp_data;
float externalTemperature;
float referenceTemperature;
float internalTemperature;
int keyState;
int option = 1;
int controlSignal = 0;
int hysteresis = 4;
int controlMode;
FILE* csv_file;
double kp = 5.0;
double ki = 1.0;
double kd = 5.0;
int trOption = 2;

const int ONOFF_CONTROL = 0;
const int PID_CONTROL = 1;
const int AUTO_CONTROL = 2;

const int OPTION_AUTO = 3;

const int TR_MANUAL = 1;
const int TR_POTENTIOMETER = 2;

int main(int argc, char* argv[]) {

    // ---------- LCD
    if (wiringPiSetup() == -1) {
        exit(1);
    }
    fd = wiringPiI2CSetup(I2C_ADDR);
    lcd_init(fd);

    // ---------- BME280
    const char I2C_BUS_ARGUMENT[] = "/dev/i2c-1";
    struct identifier id;

    if ((id.fd = open(I2C_BUS_ARGUMENT, O_RDWR)) < 0) {
        fprintf(stderr, "Failed to open the i2c bus %s\n", argv[1]);
        exit(1);
    }

    id.dev_addr = BME280_I2C_ADDR_PRIM;

    if (ioctl(id.fd, I2C_SLAVE, id.dev_addr) < 0) {
        fprintf(stderr, "Failed to acquire bus access and/or talk to slave.\n");
        exit(1);
    }

    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    dev.intf_ptr = &id;

    int8_t rslt = BME280_OK;

    rslt = bme280_init(&dev);
    if (rslt != BME280_OK) {
        fprintf(stderr, "Failed to initialize the device (code %+d).\n", rslt);
        exit(1);
    }

    rslt = stream_sensor_data_forced_mode(&dev);
    if (rslt != BME280_OK) {
        fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
        exit(1);
    }

    // ---------- UART MODBUS
    uart0_filestream = initialize_uart();

    create_pwm();

    signal(SIGINT, finishResources);
    signal(SIGSTOP, finishResources);

    pthread_create(&(threads[0]), NULL, thread_bme280, NULL);
    pthread_create(&(threads[1]), NULL, thread_uart, NULL);

    menu();

    create_csv();

    pthread_create(&(threads[2]), NULL, thread_control, NULL);
    pthread_create(&(threads[3]), NULL, thread_csv, NULL);

    while(1) {
        sleep(1);
        printf("TE %0.2lf\nTR %0.2lf\nTI %0.2lf\nKEY STATE %d\n", externalTemperature, referenceTemperature, internalTemperature, keyState);
        show_in_lcd(fd, externalTemperature, referenceTemperature, internalTemperature);
    }

    return 0;
}

void menu() {
    system("clear");
    turn_off_fan();
    turn_off_resistor();
    printf("Utilize CTRL+C ou CTRL+Z para finalizar a execução\n");
    printf("Selecione uma opção:\n(1) On/Off\n(2) PID\n(3) Utilizar estado da chave externa como controle\n(4) Para finalizar\n");
    scanf("%d", &option);
    switch (option) {
        case 1:
            controlMode = ONOFF_CONTROL;
            printf("Qual o valor desejado para a histerese?\n");
            scanf("%d", &hysteresis);
            break;

        case 2:
            controlMode = PID_CONTROL;
            printf("Qual o valor desejado para Kp?\n");
            scanf("%lf", &kp);
            printf("Qual o valor desejado para Ki?\n");
            scanf("%lf", &ki);
            printf("Qual o valor desejado para Kd?\n");
            scanf("%lf", &kd);
            pid_configura_constantes(kp, ki, kd);
            break;

        case 3:
            controlMode = AUTO_CONTROL;
            pid_configura_constantes(kp, ki, kd);
            break;

        case 4:
            finishResources();
            break;

        default:
            system("clear");
            printf("Opção errada, escolha novamente\n");
            sleep(1);
            menu();
            break;
    }
    printf("Configuração da temperatura de referencia:\n(1) Escolher temperatura de referencia\n(2) Atraves do potenciometro\n");
    scanf("%d", &trOption);
    while (trOption != TR_MANUAL && trOption != TR_POTENTIOMETER) {
        printf("Configuração da temperatura de referencia:\n(1) Escolher temperatura de referencia\n(2) Atraves do potenciometro\n");
        scanf("%d", &trOption);
    }
    if (trOption == TR_MANUAL) {
        float newReferenceTemperature;
        printf("Qual a temperatura desejada? Entre %0.2f e 100 Graus Celsius\n", externalTemperature);
        scanf("%f", &newReferenceTemperature);
        while (newReferenceTemperature < externalTemperature && newReferenceTemperature > 100.0) {
            printf("Qual a temperatura desejada? Entre %0.2f e 100 Graus Celsius\n", externalTemperature);
            scanf("%f", &newReferenceTemperature);
        }
        referenceTemperature = newReferenceTemperature;
    }
    system("clear");
}

void finishResources() {
    for (int i = 0; i < 4; i++) {
        pthread_cancel(threads[i]);
    }
    for(int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(csv_file);

    close(uart0_filestream);

    turn_off_resistor();
    turn_off_fan();

    sleep(2);

    exit(0);
}

void pid() {
    pid_atualiza_referencia(referenceTemperature);
    double newControlSignal = pid_controle(internalTemperature);
    printf("newControlSignal: %lf\n", newControlSignal);
    if (newControlSignal > 0.0) {
        if (controlSignal < 0) {
            turn_off_fan();
        }
        turn_on_resistor((int)newControlSignal);
    } else if (newControlSignal < 0.0) {
        if (controlSignal > 0) {
            turn_off_resistor();
        }
        if ((int)newControlSignal < -40) {
            turn_on_fan((int)newControlSignal*(-1));
        } else {
            turn_off_fan();
        }
    }

    controlSignal = (int)newControlSignal;
}

void on_off_control() {
    float inferiorLimit = referenceTemperature - (hysteresis/2);
    float upperLimit = referenceTemperature + (hysteresis/2);

    if (internalTemperature < inferiorLimit) {
        if (controlSignal < 0) {
            turn_off_fan();
        }
        turn_on_resistor(100);
        controlSignal = 100;
    } else if (internalTemperature > upperLimit) {
        if (controlSignal > 0) {
            turn_off_resistor();
        }
        turn_on_fan(100);
        controlSignal = -100;
    } else {
        if (controlSignal > 0) {
            turn_off_resistor();
        } else if (controlSignal < 0) {
            turn_off_fan();
        }
        controlSignal = 0;
    }
}

void *thread_bme280(void *arg) {
    while (1) {
        int8_t rslt = get_data_from_bme280(&dev, &comp_data);
        if (rslt != BME280_OK) {
            fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
            finishResources();
            exit(1);
        }
        externalTemperature = comp_data.temperature;
        sleep(1);
    }
}

void *thread_uart(void *arg) {
    while (1) {
        if (trOption == TR_POTENTIOMETER) {
            referenceTemperature = request_potentiometer_temperature(uart0_filestream);
        }
        internalTemperature = request_internal_temperature(uart0_filestream);
        if (option == OPTION_AUTO) {
            keyState = request_key_state(uart0_filestream);
            controlMode = keyState;
        }
        sendControlSignal(uart0_filestream, controlSignal);
    }
}

void *thread_control(void *arg) {
    while (1) {
        if (controlMode == ONOFF_CONTROL) {
            on_off_control();
        } else if (controlMode == PID_CONTROL){
            pid();
        }
        sleep(1);
    }
}

void *thread_csv(void *arg) {
    while (1) {
        sleep(2);
        add_data_in_csv();
    }
}

void create_csv() {
    csv_file = fopen("project1.csv", "w");
    fprintf(csv_file, "Data/Hora, Temperatura Externa, Temperatura Interna, Temperatura Referencia, Sinal de Controle\n");
    fclose(csv_file);
}

void add_data_in_csv() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    csv_file = fopen("project1.csv", "a+");

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%F %X", timeinfo);

    fprintf(csv_file, "%s, %.3f, %.3f, %.3f, %d%%\n", buffer, externalTemperature, internalTemperature, referenceTemperature, controlSignal);

    fclose(csv_file);
}