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

pthread_t threads[3];
int fd;
int uart0_filestream;
struct bme280_dev dev;
struct bme280_data comp_data;
float externalTemperature;
float referenceTemperature;
float internalTemperature;
int keyState;
int op = 1;
int controlSignal = 0;

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

    pid_configura_constantes(5.0, 3.0, 3.0);

    signal(SIGINT, finishResources);

    pthread_create(&(threads[0]), NULL, thread_bme280, NULL);
    pthread_create(&(threads[1]), NULL, thread_uart, NULL);
    pthread_create(&(threads[2]), NULL, thread_control, NULL);

    while(1) {
        printf("TE %0.2lf\nTR %0.2lf\nTI %0.2lf\nKEY STATE %d\n", externalTemperature, referenceTemperature, internalTemperature, keyState);
        show_in_lcd(fd, externalTemperature, referenceTemperature, internalTemperature);
        sleep(1);
    }

    return 0;
}

void finishResources() { // TODO: CLOSE RESOURCES EM TODOS EXITS E SIGINT
    turn_off_resistor();
    turn_off_fan();
    close(uart0_filestream);
    for(int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
}

int pid(float referenceTemperature, float internalTemperature, int controlSignal) {
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

    return (int)newControlSignal;
}

int on_off_control(float referenceTemperature, float internalTemperature, int controlSignal) {
    const int HYSTERESIS = 4;
    float inferiorLimit = referenceTemperature - (HYSTERESIS/2);
    float upperLimit = referenceTemperature + (HYSTERESIS/2);

    int newControlSignal;

    if (internalTemperature < inferiorLimit) {
        if (controlSignal < 0) {
            turn_off_fan();
        }
        turn_on_resistor(100);
        newControlSignal = 100;
    } else if (internalTemperature > upperLimit) {
        if (controlSignal > 0) {
            turn_off_resistor();
        }
        turn_on_fan(100);
        newControlSignal = -100;
    } else {
        if (controlSignal > 0) {
            turn_off_resistor();
        } else if (controlSignal < 0) {
            turn_off_fan();
        }
        newControlSignal = 0;
    }

    return newControlSignal;
}

void *thread_bme280(void *arg) {
    while (1) {
        int8_t rslt = get_data_from_bme280(&dev, &comp_data);
        if (rslt != BME280_OK) {
            fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
            exit(1);
        }
        externalTemperature = comp_data.temperature;
        sleep(1);
    }
}

void *thread_uart(void *arg) {
    while (1) {
        referenceTemperature = request_potentiometer_temperature(uart0_filestream);
        internalTemperature = request_internal_temperature(uart0_filestream);
        keyState = request_key_state(uart0_filestream);
        sendControlSignal(uart0_filestream, controlSignal);
    }
}

void *thread_control(void *arg) {
    printf("Selecione entre On/Off (1) e PID (2)\n");
    scanf("%d", &op);
    turn_off_fan();
    turn_off_resistor();

    while (1) {
        if (op == 1) {
            controlSignal = on_off_control(referenceTemperature, internalTemperature, controlSignal);
        } else {
            controlSignal = pid(referenceTemperature, internalTemperature, controlSignal);
        }
        sleep(1);
    }
}