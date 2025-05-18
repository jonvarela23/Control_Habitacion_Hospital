/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: ADC.h
 *
 * Descripción: Driver para la lectura de adc
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef MAIN_ADC_H_
#define MAIN_ADC_H_

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************

//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************

void adc_simple_init();
int adc_simple_read_raw();
int adc_simple_read_mv();

#endif /* MAIN_ADC_H_ */
