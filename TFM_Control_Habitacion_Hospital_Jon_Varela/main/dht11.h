/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 * 
 * Archivo: dht11.h
 * 
 * Descripción: Driver para la lectura de temperatura y humedad del sensor dht11
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef DHT11_H_
#define DHT11_H_

#include "driver/gpio.h"

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************

#define DHT11_GPIO 		22

enum dht11_status {
    DHT11_CRC_ERROR = -2,
    DHT11_TIMEOUT_ERROR,
    DHT11_OK
};

struct dht11_reading {
    int status;
    int temperature;
    int humidity;
};

//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************

void DHT11_init(gpio_num_t);

struct dht11_reading DHT11_read();

#endif
