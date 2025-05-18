/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: lcd_16x2.h
 *
 * Descripción: Driver para la escritura en el LCD de 16x2
 *
 * Autor: Jon Varela Gonzalez
*/

#pragma once

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************

#define LCD_ADDR 0x3F
#define SDA_PIN  26
#define SCL_PIN  27
#define LCD_COLS 16
#define LCD_ROWS 2

//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************

void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows);
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_home(void);
void LCD_clearScreen(void);
void LCD_writeChar(char c);
void LCD_writeStr(char* str); 
