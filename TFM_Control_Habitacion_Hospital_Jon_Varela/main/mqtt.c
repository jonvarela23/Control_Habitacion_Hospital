/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: mqtt.c
 *
 * Descripción: Driver para la comunicación mqtt
 *
 * Autor: Jon Varela Gonzalez
*/

// Include standard lib headers
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

// Include ESP submodules headers.
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

// Include own project  headers
#include "mqtt.h"

#include "ADC.h"
#include "general.h"
#include "inputs_outputs.h"
#include "json.h"

//****************************************************************************
//      Variables de este fichero
//****************************************************************************

static const char *TAG = "MQTT";

esp_mqtt_client_handle_t client = NULL;

char topic_recepcion[64];
char topic_envio[64];

uint8_t Error_PC 				= 0;
uint8_t Error_Habitacion		= 0;

static TaskHandle_t TaskHandlerSenalizacionErrorPC = NULL;
static TaskHandle_t TaskHandlerSenalizacionErrorHabitacion = NULL;

//****************************************************************************
//      Variables externas inicializadas en otro fichero
//****************************************************************************

extern uint8_t Actuacion_Termostato;
extern uint8_t Actuacion_Persianas;
extern uint8_t Actuacion_Ruido;

extern uint8_t Numero_Habitacion;

extern uint8_t Estado_sistema;

extern uint8_t Minute;
extern uint8_t Hour;
extern uint8_t Second;

//****************************************************************************
//		Funciones
//****************************************************************************
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

static void Senalizacion_ErrorPC(void *pvParameters);

static void Senalizacion_ErrorHabitacion(void *pvParameters);


// Callback that will handle MQTT events. Will be called by  the MQTT internal task.
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
        {
        	char buffer[30];
        	struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client,"/TFM/jvg/PC/#", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // Si se ha conectado correctamente, se envía un mensaje de conexión retenido para determinar si está conectado o no cuando otro cliente se conecta al Broker
			json_printf(&out1,"{Habitacion:conectado}\0");
			esp_mqtt_client_publish(client,topic_envio, buffer, 0, 0, 1);

			Error_Habitacion = 0;

			if (TaskHandlerSenalizacionErrorHabitacion != NULL){
				vTaskDelete(TaskHandlerSenalizacionErrorHabitacion);
				TaskHandlerSenalizacionErrorHabitacion = NULL;
			}

			if (Estado_sistema != LLAMADA)
			{
				gpio_set_level(LED_Llamada_Paciente, 0);
			}

            break;
        }
        case MQTT_EVENT_DISCONNECTED:
        {
        	char buffer[30];
        	struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

			json_printf(&out1,"{Habitacion:desconectado}\0");
			esp_mqtt_client_publish(client,topic_envio, buffer, 0, 0, 1);

			Error_Habitacion = 1;

			if (TaskHandlerSenalizacionErrorHabitacion == NULL){
				xTaskCreate(Senalizacion_ErrorHabitacion, "Senalizacion_ErrorHabitacion", 1024, NULL, 1, &TaskHandlerSenalizacionErrorHabitacion);
			}

            break;
        }
        case MQTT_EVENT_SUBSCRIBED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        }
        case MQTT_EVENT_UNSUBSCRIBED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        }
        case MQTT_EVENT_PUBLISHED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        }
        case MQTT_EVENT_DATA:
        {
        	// Para poder imprimir el nombre del topic lo tengo que copiar en una cadena correctamente terminada
        	char topic_name[event->topic_len+1]; // Esto va a la pila y es potencialmente peligroso si el nombre del topic es grande
        	strncpy(topic_name,event->topic,event->topic_len);
        	topic_name[event->topic_len]=0; // Añade caracter de terminacion al final

#ifdef DEBUG
        	ESP_LOGI(TAG, "MQTT_EVENT_DATA: Topic %s",topic_name);
#endif /*DEBUG*/

        	char mensaje[30];

			uint8_t Hora, Minuto, Segundo, Estado;

        	if ((strncmp(topic_name,topic_recepcion,23) == 0) || (strncmp(topic_name,topic_recepcion,24) == 0)) // Le llega un mensaje al Topic suscrito: "/TFM/jvg/PC/HabitacionX" donde X es el número de habitacion, que puede ser de 1 a 16
        	{

				if(json_scanf(event->data, event->data_len, "{ActuarTermostato: %d }", &Estado) == 1) // Mensaje de ActuarTemp para activar o desactivar la actuación automática del termostato en función de la temperatura
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "ActuarTermostato: %s", Estado ? "true":"false");
#endif /*DEBUG*/

					Actuacion_Termostato = Estado;

				}

				if(json_scanf(event->data, event->data_len, "{ActuarRuido: %d }", &Estado) == 1) // Mensaje de ActuarRuido para activar o desactivar la actuación automática de la llamada emergencia en función del ruido
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "ActuarRuido: %s", Estado ? "true":"false");
#endif /*DEBUG*/

					Actuacion_Ruido = Estado;

				}

				if(json_scanf(event->data, event->data_len, "{ActuarLEDRuido: %d }", &Estado) == 1) // Mensaje de ActuarLEDRuido para activar o desactivar la actuación sobre el LED cuando hay demasiado ruido
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "ActuarLEDRuido: %s", Estado ? "true":"false");
#endif /*DEBUG*/

				}

				if(json_scanf(event->data, event->data_len, "{ActuarPersianas: %d }", &Estado) == 1) // Mensaje de ActuarPersianas para activar o desactivar la actuación automática de las persianas en función de la luz y hora
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "ActuarPersianas: %s", Estado ? "true":"false");
#endif /*DEBUG*/

					Actuacion_Persianas = Estado;

				}

				if(json_scanf(event->data, event->data_len, "{PresenciaPC: %d }", &Estado) == 1) // Mensaje de PresenciaPC para activar o desactivar el estado de Presencia desde la aplicación de PC
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "PresenciaPC: %s", Estado ? "true":"false");
#endif /*DEBUG*/

					if (Estado == 1)
					{
						if (Estado_sistema == IDLE || Estado_sistema == LLAMADA)
						{
							char buffer[100];

							struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

							gpio_set_level(LED_Presencia_Asistencia, 1);
							gpio_set_level(LED_Llamada_Paciente, 0);
							Estado_sistema = PRESENCIA;

							json_printf(&out1," {Presencia: %B ,",1);
							json_printf(&out1," LlamadaPaciente: %B ,",0);
							json_printf(&out1," LlamadaPacienteRuido: %B }",0);

							int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
							ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

						}
						else if (Estado_sistema == PRESENCIA)
						{
							char buffer[50];

							struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

							gpio_set_level(LED_Presencia_Asistencia, 1);

							json_printf(&out1," {Presencia: %B }",1);

							int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
							ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/
						}
					}
					else if (Estado == 0)
					{
						char buffer[50];

						struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

						gpio_set_level(LED_Presencia_Asistencia, 0);
						gpio_set_level(LED_Llamada_Paciente, 0);

						Estado_sistema = IDLE;

						json_printf(&out1," {Presencia: %B }",0);

						int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
						ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/
					}

				}

				if(json_scanf(event->data, event->data_len, "{Hora: %d }", &Hora) == 1) // Mensaje de Hora para cambiar la hora del nodo
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "Hora: %u", Hora);
#endif /*DEBUG*/

					Hour = Hora;

				}

				if(json_scanf(event->data, event->data_len, "{Minuto: %d }", &Minuto) == 1) // Mensaje de Minuto para cambiar los minutos del nodo
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "Minuto: %u", Minuto);
#endif /*DEBUG*/

					Minute = Minuto;

				}

				if(json_scanf(event->data, event->data_len, "{Segundo: %d }", &Segundo) == 1) // Mensaje de Segundo para cambiar los segundos del nodo
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "Segundo: %u", Segundo);
#endif /*DEBUG*/

					Second = Segundo;

				}
        	}

        	if (strncmp(topic_name,"/TFM/jvg/PC",11) == 0) // Le llega un mensaje al Topic: "/TFM/jvg/PC"
        	{
        		if(json_scanf(event->data, event->data_len, "{PC: %s }", &mensaje) == 1) // Mensaje de PC Conectado o Desconectado
				{

#ifdef DEBUG
					ESP_LOGI(TAG, "PC: %s", mensaje);
#endif /*DEBUG*/

					if (strncmp(mensaje, "Desconectado",12) == 0) // Si la aplicación de PC se ha desconectado
					{

						Error_PC = 1;

						if (TaskHandlerSenalizacionErrorPC == NULL){
							xTaskCreate(Senalizacion_ErrorPC, "Senalizacion_ErrorPC", 1024, NULL, 1, &TaskHandlerSenalizacionErrorPC);
						}

					}

					if (strncmp(mensaje, "Conectado",9) == 0) // Si la aplicación de PC se ha conectado
					{

						Error_PC = 0;

						if (TaskHandlerSenalizacionErrorPC != NULL){
							vTaskDelete(TaskHandlerSenalizacionErrorPC);
							TaskHandlerSenalizacionErrorPC = NULL;
						}

						char buffer[100];

						struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

						if (Estado_sistema == LLAMADA)
						{
							gpio_set_level(LED_Presencia_Asistencia, 0);
							gpio_set_level(LED_Llamada_Paciente, 1);

							json_printf(&out1," {Presencia: %B ,",0);
							json_printf(&out1," LlamadaPaciente: %B ,",1);
							json_printf(&out1," LlamadaPacienteRuido: %B }",0);
						}
						else if (Estado_sistema == PRESENCIA)
						{
							gpio_set_level(LED_Presencia_Asistencia, 1);
							gpio_set_level(LED_Llamada_Paciente, 0);

							json_printf(&out1," {Presencia: %B ,",1);
							json_printf(&out1," LlamadaPaciente: %B ,",0);
							json_printf(&out1," LlamadaPacienteRuido: %B }",0);
						}
						else
						{
							gpio_set_level(LED_Presencia_Asistencia, 0);
							gpio_set_level(LED_Llamada_Paciente, 0);

							json_printf(&out1," {Presencia: %B ,",0);
							json_printf(&out1," LlamadaPaciente: %B ,",0);
							json_printf(&out1," LlamadaPacienteRuido: %B }",0);
						}

						int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
						ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

					}

				}

        	}

        	break;

		}

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

// Tarea de señalización en caso de error de conexión de la aplicación de PC
static void Senalizacion_ErrorPC(void *pvParameters)
{
	uint8_t Estado = 0;

	while (1)
	{
		if (Error_PC == 1)
		{
			if (Estado == 0)
			{
				gpio_set_level(LED_Presencia_Asistencia, 1);
				Estado = 1;
			}
			else
			{
				gpio_set_level(LED_Presencia_Asistencia, 0);
				Estado = 0;
			}
		}

		vTaskDelay(0.5*configTICK_RATE_HZ); // Se espera durante 500ms

	}

}

// Tarea de señalización en caso de error de conexión del nodo de habitación
static void Senalizacion_ErrorHabitacion(void *pvParameters)
{
	uint8_t Estado = 0;

	while (1)
	{
		if (Error_Habitacion == 1)
		{
			if (Estado == 0)
			{
				gpio_set_level(LED_Llamada_Paciente, 1);
				Estado = 1;
			}
			else
			{
				gpio_set_level(LED_Llamada_Paciente, 0);
				Estado = 0;
			}
		}

		vTaskDelay(0.5*configTICK_RATE_HZ); // Se espera durante 500ms

	}

}

esp_err_t mqtt_app_start(const char* url)
{
	esp_err_t error;

	char buffer[30];

	struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

	sprintf(topic_envio, "/TFM/jvg/ESP/Habitacion%d", Numero_Habitacion);

	sprintf(topic_recepcion, "/TFM/jvg/PC/Habitacion%d", Numero_Habitacion);

	xTaskCreate(Senalizacion_ErrorPC, "Senalizacion_ErrorPC", 1024, NULL, 1, &TaskHandlerSenalizacionErrorPC);
	xTaskCreate(Senalizacion_ErrorHabitacion, "Senalizacion_ErrorHabitacion", 1024, NULL, 1, &TaskHandlerSenalizacionErrorHabitacion);

	if (client==NULL){

		json_printf(&out1,"{Habitacion:desconectado}\0");

		// Se inicia el cliente MQTT con un mensaje de última voluntad retenido para determinar si está conectado o no cuando otro cliente se conecta al Broker
		esp_mqtt_client_config_t mqtt_cfg = {
				.broker.address.uri = CONFIG_EXAMPLE_MQTT_BROKER_URI,
				.broker.address.port = 1883,
				.session.last_will.topic = topic_envio,
				.session.last_will.msg = buffer,
				.session.last_will.retain = 1,
				.session.keepalive = 4,
		};

		if(url[0] != '\0'){
			mqtt_cfg.broker.address.uri= url;
		}


		client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
		error=esp_mqtt_client_start(client);

		return error;
	}
	else {

		ESP_LOGE(TAG,"MQTT client already running");
		return ESP_FAIL;
	}

}
