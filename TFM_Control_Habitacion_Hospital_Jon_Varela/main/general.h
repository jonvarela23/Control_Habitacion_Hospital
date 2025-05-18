/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: general.h
 *
 * Descripción: Archivo general para las variables compartidas por archivos
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef MAIN_GENERAL_H_
#define MAIN_GENERAL_H_

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************

#define SUBIR_PERSIANAS_FLAG 	0x0001 	// Evento para el flag de subir persianas
#define BAJAR_PERSIANAS_FLAG 	0x0002 	// Evento para el flag de bajar persianas
#define SUBIR_CORTINAS_FLAG 	0x0004 	// Evento para el flag de subir cortinas
#define BAJAR_CORTINAS_FLAG 	0x0008 	// Evento para el flag de bajar cortinas

#define DEBOUNCE_TIME_MS 		100		// Cantidad de antirebote de las inputs

//#define DEBUG

enum Estados{
	IDLE = 0,
	LLAMADA,
	PRESENCIA
};

#endif /* MAIN_GENERAL_H_ */
