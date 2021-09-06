#ifndef UART_MODBUS_H_
#define UART_MODBUS_H_

int initialize_uart();

void write_in_uart(int uart0_filestream, char device_code, char code, char subcode);

void read_data_uart(int uart0_filestream, unsigned char *rx_buffer);

float request_internal_temperature(int uart0_filestream);

float request_potentiometer_temperature(int uart0_filestream);

int request_key_state(int uart0_filestream);

void sendControlSignal(int uart0_filestream, int controlSignal);

#endif // UART_MODBUS_H_
