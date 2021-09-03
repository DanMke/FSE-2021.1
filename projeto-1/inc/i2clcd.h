#ifndef I2CLCD_H_
#define I2CLCD_H_

// Define some device parameters
#define I2C_ADDR   0x27 // I2C device address

// Define some device constants
#define LCD_CHR  1 // Mode - Sending data
#define LCD_CMD  0 // Mode - Sending command

#define LINE1  0x80 // 1st line
#define LINE2  0xC0 // 2nd line

#define LCD_BACKLIGHT   0x08  // On
// LCD_BACKLIGHT = 0x00  # Off

#define ENABLE  0b00000100 // Enable bit

void lcd_init(int fd);
void lcd_byte(int bits, int mode, int fd);
void lcd_toggle_enable(int bits, int fd);

// added by Lewis
void typeInt(int i, int fd);
void typeFloat(float myFloat, int fd);
void lcdLoc(int line, int fd); //move cursor
void ClrLcd(int fd); // clr LCD return home
void typeln(const char *s, int fd);
void typeChar(char val, int fd);

#endif //I2CLCD_H_
