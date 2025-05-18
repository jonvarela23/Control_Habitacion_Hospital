/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: wifi.h
 *
 * Descripción: Driver para la comunicación wifi
 *
 * Autor: Jon Varela Gonzalez
*/

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

//*****************************************************************************
//      PROTOTIPOS DE FUNCIONES
//*****************************************************************************
extern void wifi_init_sta(void);
extern void wifi_init_softap(void);
extern void wifi_initlib();
extern void wifi_change_to_AP(void);
extern void wifi_change_to_sta(void);

#endif /* MAIN_WIFI_H_ */
