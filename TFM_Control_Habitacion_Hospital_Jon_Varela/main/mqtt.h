/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: mqtt.h
 *
 * Descripción: Driver para la comunicación mqtt
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef __MQTT_H__
#define __MQTT_H__

//*****************************************************************************
//      DEFINICIONES
//*****************************************************************************


//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************

esp_err_t mqtt_app_start(const char* url);

#endif //  __MQTT_H__
