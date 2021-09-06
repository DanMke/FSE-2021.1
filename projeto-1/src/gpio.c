#include "../inc/gpio.h"

#include <softPwm.h>
#include <wiringPi.h>

const int PIN_RESISTOR_GPIO = 4;
const int PIN_FAN_GPIO = 5;

void create_pwm() {
    pinMode(PIN_RESISTOR_GPIO, OUTPUT);
    softPwmCreate(PIN_RESISTOR_GPIO, 0, 100);
    pinMode(PIN_FAN_GPIO, OUTPUT);
    softPwmCreate(PIN_FAN_GPIO, 0, 100);
}

void turn_on_resistor(int intensity) {
    softPwmWrite(PIN_RESISTOR_GPIO, intensity);
}

void turn_on_fan(int intensity) {
    softPwmWrite(PIN_FAN_GPIO, intensity);
}

void turn_off_resistor() {
    softPwmWrite(PIN_RESISTOR_GPIO, 0);
}

void turn_off_fan() {
    softPwmWrite(PIN_FAN_GPIO, 0);
}