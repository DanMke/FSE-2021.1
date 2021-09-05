#include "../inc/gpio.h"

#include <softPwm.h>
#include <wiringPi.h>

void turn_on_resistor(int intensity) {
    const int PIN_RESISTOR_GPIO = 4;

    pinMode(PIN_RESISTOR_GPIO, OUTPUT);
    softPwmCreate(PIN_RESISTOR_GPIO, 0, 100);
    softPwmWrite(PIN_RESISTOR_GPIO, intensity);
}

void turn_on_fan(int intensity) {
    const int PIN_FAN_GPIO = 5;

    pinMode(PIN_FAN_GPIO, OUTPUT);
    softPwmCreate(PIN_FAN_GPIO, 0, 100);
    softPwmWrite(PIN_FAN_GPIO, intensity);
}

void turn_off_resistor() {
    const int PIN_RESISTOR_GPIO = 4;

    pinMode(PIN_RESISTOR_GPIO, OUTPUT);
    softPwmCreate(PIN_RESISTOR_GPIO, 0, 100);
    softPwmWrite(PIN_RESISTOR_GPIO, 0);
}

void turn_off_fan() {
    const int PIN_FAN_GPIO = 5;

    pinMode(PIN_FAN_GPIO, OUTPUT);
    softPwmCreate(PIN_FAN_GPIO, 0, 100);
    softPwmWrite(PIN_FAN_GPIO, 0);
}