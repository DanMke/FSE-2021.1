#ifndef GPIO_H_
#define GPIO_H_

void turn_on_resistor(int intensity);
void turn_on_fan(int intensity);
void turn_off_resistor();
void turn_off_fan();
void create_pwm();

#endif //GPIO_H_
