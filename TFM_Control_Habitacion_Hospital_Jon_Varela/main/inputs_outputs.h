/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: inputs_outputs.h
 *
 * Descripción: Driver para las entradas y salidas (GPIO) y para los LEDs tanto lógicos como de PWM
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef GPIO_LEDS_H_
#define GPIO_LEDS_H_

#include <stdint.h>

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************

#define LED_Llamada_Paciente 		18
#define LED_Presencia_Asistencia 	19
#define LED_Termostato 				21
#define LED_Persianas 				0
#define LED_Cortinas 				2
#define LED_Luz_Habitacion 			4

#define Boton_Llamada_Paciente 		12
#define Boton_Presencia_Asistencia 	13
#define Boton_Subir_Persianas 		14
#define Boton_Bajar_Persianas 		15
#define Boton_Subir_Cortinas 		35
#define Boton_Bajar_Cortinas	 	36

#define Input_Sensor_Ruido	 		23
#define Input_Sensor_Luz		 	39

#define Input_switch		 		25

#define GPIO_INPUT_PIN_Llamada_Paciente  		(1ULL<<Boton_Llamada_Paciente)
#define GPIO_INPUT_PIN_Presencia_Asistencia  	(1ULL<<Boton_Presencia_Asistencia)
#define GPIO_INPUT_PIN_Subir_Persianas  		(1ULL<<Boton_Subir_Persianas)
#define GPIO_INPUT_PIN_Bajar_Persianas  		(1ULL<<Boton_Bajar_Persianas)
#define GPIO_INPUT_PIN_Subir_Cortinas  			(1ULL<<Boton_Subir_Cortinas)
#define GPIO_INPUT_PIN_Bajar_Cortinas  			(1ULL<<Boton_Bajar_Cortinas)

#define GPIO_INPUT_PIN_Sensor_Ruido 			(1ULL<<Input_Sensor_Ruido)
#define GPIO_INPUT_PIN_Sensor_Luz	  			(1ULL<<Input_Sensor_Luz)

#define GPIO_INPUT_PIN_Switch 					(1ULL<<Input_switch)

#define ESP_INTR_FLAG_DEFAULT 0

//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************

void GL_initGPIO(void);
void GL_initLEDC();

void GL_setPersianas(uint8_t Level);
void GL_setCortinas(uint8_t Level);
void GL_setHabitacion(uint8_t Level);

void gpio_isr_handler(void* arg);

#endif // GPIO_LEDS_H_
