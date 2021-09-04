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

#include "bme280.h"
#include "i2clcd.h"
#include "crc16.h"
#include "uart_modbus.h"

const char I2C_BUS_ARGUMENT[] = "/dev/i2c-1";

void loop(struct bme280_dev dev, int fd, int uart0_filestream);

int main(int argc, char* argv[]) {

    struct bme280_dev dev;

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

    // ----------   initialize lcd -----------------

    if (wiringPiSetup () == -1) {
        exit (1);
    }

    int fd = wiringPiI2CSetup(I2C_ADDR);

    lcd_init(fd); // setup LCD

    // ----------   modbus -----------------

    int uart0_filestream = initialize_uart();

    // ----------   loop -----------------

    loop(dev, fd, uart0_filestream);

    close(uart0_filestream);

    return 0;
}

void loop(struct bme280_dev dev, int fd, int uart0_filestream) {
    struct bme280_data comp_data;

    while (1) {
        int8_t rslt = get_data_from_bme280(&dev, &comp_data);
        if (rslt != BME280_OK) {
            fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
            exit(1);
        }

        float externalTemperature = comp_data.temperature;
        float referenceTemperature = request_potentiometer_temperature(uart0_filestream);
        float internalTemperature = request_internal_temperature(uart0_filestream);
        int keyState = request_key_state(uart0_filestream);

        printf("TE %0.2lf\n", externalTemperature);
        printf("TR %0.2lf\n", referenceTemperature);
        printf("TI %0.2lf\n", internalTemperature);
        printf("KEY STATE %d\n", keyState);

        show_in_lcd(fd, externalTemperature, referenceTemperature, internalTemperature);

        sleep(2);
    }
}