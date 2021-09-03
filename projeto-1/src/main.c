/**\
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/**
 * \ingroup bme280
 * \defgroup bme280Examples Examples
 * @brief Reference Examples
 */

/*!
 * @ingroup bme280Examples
 * @defgroup bme280GroupExampleLU linux_userspace
 * @brief Linux userspace test code, simple and mose code directly from the doco.
 * compile like this: gcc linux_userspace.c ../bme280.c -I ../ -o bme280
 * tested: Raspberry Pi.
 * Use like: ./bme280 /dev/i2c-0
 * \include linux_userspace.c
 */

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

/******************************************************************************/
/*!                         System header files                               */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <wiringPiI2C.h>
#include <wiringPi.h>

#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

/******************************************************************************/
/*!                         Own header files                                  */
#include "bme280.h"
#include "i2clcd.h"
#include "crc16.h"

void showInLCD(int fd, float externalTemperature, float referenceTemperature, float internalTemperature);

/*!
 * @brief Function reads temperature, humidity and pressure data in forced mode.
 *
 * @param[in] dev   :   Structure instance of bme280_dev.
 *
 * @return Result of API execution status
 *
 * @retval BME280_OK - Success.
 * @retval BME280_E_NULL_PTR - Error: Null pointer error
 * @retval BME280_E_COMM_FAIL - Error: Communication fail error
 * @retval BME280_E_NVM_COPY_FAILED - Error: NVM copy failed
 *
 */
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev);

int8_t get_data_from_bme280(struct bme280_dev *dev, struct bme280_data *comp_data);

void loop(struct bme280_dev dev, int fd);

/*!
 * @brief This function starts execution of the program.
 */
int main(int argc, char* argv[]) {

    struct bme280_dev dev;

    struct identifier id;

    if (argc < 2) {
        fprintf(stderr, "Missing argument for i2c bus.\n");
        exit(1);
    }

    if ((id.fd = open(argv[1], O_RDWR)) < 0) {
        fprintf(stderr, "Failed to open the i2c bus %s\n", argv[1]);
        exit(1);
    }

    /* Make sure to select BME280_I2C_ADDR_PRIM or BME280_I2C_ADDR_SEC as needed */
    id.dev_addr = BME280_I2C_ADDR_PRIM;

    if (ioctl(id.fd, I2C_SLAVE, id.dev_addr) < 0) {
        fprintf(stderr, "Failed to acquire bus access and/or talk to slave.\n");
        exit(1);
    }

    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    /* Update interface pointer with the structure that contains both device address and file descriptor */
    dev.intf_ptr = &id;

    /* Variable to define the result */
    int8_t rslt = BME280_OK;

    /* Initialize the bme280 */
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

    tx_buffer[0] = 0x01;
    tx_buffer[1] = 0x23;
    tx_buffer[2] = 0xC1;

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
            short crc = calcula_CRC(rx_buffer, rx_length - 2);
            short oldCrc;
            memcpy(&oldCrc, rx_buffer + (rx_length - 2), 2);

            if (crc != oldCrc) {
                close(uart0_filestream);
                exit(1);
            }

            //Bytes received
            rx_buffer[rx_length] = '\0';
//            int currentInteger;
//            memcpy(&currentInteger, rx_buffer, rx_length);
//            printf("INT %d: %d\n", rx_length, currentInteger);
//            printf("%i Bytes lidos : %d\n", rx_length, currentInteger);
            float currentFloat;
            memcpy(&currentFloat, rx_buffer+3, 4);
            printf("FLOAT %d: %f\n", rx_length, currentFloat);
            printf("%i Bytes lidos : %f\n", rx_length, currentFloat);
        }
    }

    close(uart0_filestream);

    // ----------   loop -----------------

    loop(dev, fd);

    return 0;
}

void showInLCD(int fd, float externalTemperature, float referenceTemperature, float internalTemperature) {
    ClrLcd(fd);

    lcdLoc(LINE1, fd);
    typeln("TE", fd);
    typeFloat(externalTemperature, fd);

    typeln(" TR", fd);
    typeFloat(referenceTemperature, fd);

    lcdLoc(LINE2, fd);
    typeln("TI", fd);
    typeFloat(internalTemperature, fd);

    delay(2000);
}

/*!
 * @brief This API reads the sensor temperature, pressure and humidity data in forced mode.
 */
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev)
{
    /* Variable to define the result */
    int8_t rslt = BME280_OK;

    /* Variable to define the selecting sensors */
    uint8_t settings_sel = 0;

    /* Recommended mode of operation: Indoor navigation */
    dev->settings.osr_h = BME280_OVERSAMPLING_1X;
    dev->settings.osr_p = BME280_OVERSAMPLING_16X;
    dev->settings.osr_t = BME280_OVERSAMPLING_2X;
    dev->settings.filter = BME280_FILTER_COEFF_16;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    /* Set the sensor settings */
    rslt = bme280_set_sensor_settings(settings_sel, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to set sensor settings (code %+d).", rslt);

        return rslt;
    }

    return rslt;
}

int8_t get_data_from_bme280(struct bme280_dev *dev, struct bme280_data *comp_data) {

    /* Variable to store minimum wait time between consecutive measurement in force mode */
    /*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
     *  and the oversampling configuration. */
    uint32_t req_delay = bme280_cal_meas_delay(&dev->settings);

    /* Continuously stream sensor data */
    /* Set the sensor to forced mode */
    int8_t rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to set sensor mode (code %+d).", rslt);
        return rslt;
    }

    /* Wait for the measurement to complete and print data */
    dev->delay_us(req_delay, dev->intf_ptr);
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to get sensor data (code %+d).", rslt);
        return rslt;
    }

    printf("Temperature, Pressure, Humidity\n");

    print_sensor_data(comp_data);

    return rslt;
}

void loop(struct bme280_dev dev, int fd) {
    /* Structure to get the pressure, temperature and humidity values */
    struct bme280_data comp_data;

    while (1) {
        int8_t rslt = get_data_from_bme280(&dev, &comp_data);
        if (rslt != BME280_OK) {
            fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
            exit(1);
        }

        printf("testando %0.2lf deg C\n", comp_data.temperature);

        float externalTemperature = 25.55;
        float referenceTemperature = 26.66;
        float internalTemperature = 27.77;
        showInLCD(fd, externalTemperature, referenceTemperature, internalTemperature);

        sleep(2);
    }
}